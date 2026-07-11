'use strict';

import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';

//returns the last index of a string/char-literal/comment starting at text[i], text.length if it runs to end-of-line, or -1 if text[i] doesn't start one.
function skipLiteral( text:string, i:number ):number {
	const ch = text[i];
	if( ch == '/' ){
		if( text[i+1] == '/' )
			return text.length;
		if( text[i+1] != '*' )
			return -1;
		const close = text.indexOf( '*/', i+2 );
		return close < 0 ? text.length : close+1;//a block comment spanning lines still leaks: later lines are scanned as code.
	}
	if( ch == '"' ){
		if( text[i-1] == 'R' ){//raw string R"delim( ... )delim"
			const open = text.indexOf( '(', i+1 );
			if( open < 0 )
				return text.length;
			const close = text.indexOf( ')'+text.substring(i+1, open)+'"', open+1 );
			return close < 0 ? text.length : close+open-i;
		}
		for( let j=i+1; j<text.length; ++j ){
			if( text[j] == '\\' )
				++j;
			else if( text[j] == '"' )
				return j;
		}
		return text.length;//unterminated - stop scanning the line; treating the rest as code corrupts strings & looped forever pre-fix.
	}
	if( ch == "'" ){
		for( let j=i+1; j<text.length && j<i+12; ++j ){//cap: char literals are short & an unpaired ' is a digit separator (1'000).
			if( text[j] == '\\' )
				++j;
			else if( text[j] == "'" )
				return j;
		}
	}
	return -1;
}

function parseInner( line:vscode.TextLine, index:{index:number} ):vscode.TextEdit[] {
	let edits:vscode.TextEdit[] = [];
	const deleted = new Set<number>();//offsets already queued for deletion; a single-space inner pair (`f(( ))`) hits the same space from both the after-open & before-close rules, and VS Code rejects an overlapping edit set outright.
	const deleteChar = ( at:number ):void => {
		if( deleted.has(at) )
			return;
		deleted.add( at );
		edits.push( vscode.TextEdit.delete( new vscode.Range(line.range.start.translate(0, at), line.range.start.translate(0, at+1))) );
	};
	let i = index.index;
	if( ++i<line.text.length && line.text[i] != ' ' && line.text[i] != ')' && line.text[i] != '}' )//`< length` not `length-1`: the char after the bracket may be the last on the line (`f(x` -> `f( x`).
		edits.push( vscode.TextEdit.insert(line.range.start.translate(0, i), ' ') );
	let innerBraceCount = 0;
	for( ; i<line.text.length; ++i ){
		const iLiteralEnd = skipLiteral( line.text, i );
		if( iLiteralEnd >= 0 ){
			i = iLiteralEnd;
			continue;
		}
		let ch = line.text[i];
		if( ch == '(' || ch == '{' ){
			++innerBraceCount;
			if( i+1 < line.text.length && line.text[i+1] == ' ' )//`i+1 < length`: delete the space even when it is the last char (`f(g( ` at EOL); the delete range end at col=length is valid.
				deleteChar( i+1 );
		}
		else if( ch == ')' || ch == '}' ){
			--innerBraceCount;
			if( innerBraceCount < 0 ){
				if( i>0 && line.text[i-1] != ' ' && line.text[i-1] != '(' && line.text[i-1] != '{' )
					edits.push( vscode.TextEdit.insert(line.range.start.translate( 0, i ), ' ') );
				break;
			}
			else if( i>0 && line.text[i-1] == ' ' )
				deleteChar( i-1 );
		}
	}
	index.index = i;
	return edits;
}
function parseLine( line:vscode.TextLine ):vscode.TextEdit[]{
	let edits:vscode.TextEdit[] = [];
	for( let i=0; i<line.text.length; ++i ){
		const iLiteralEnd = skipLiteral( line.text, i );
		if( iLiteralEnd >= 0 ){
			i = iLiteralEnd;
			continue;
		}
		let ch = line.text[i];
		if( ch == '(' || ch == '{' ){
			let index = {index:i};
			edits.push( ...parseInner(line, index) );
			i = index.index;
		}
	}
	return edits;
}

function findRepoRoot( start:string ):string {
	let dir = fs.realpathSync( start );
	for( ;; ){
		if( fs.existsSync(path.join(dir, '.git')) )
			return dir;
		const parent = path.dirname( dir );
		if( parent == dir )
			return start;
		dir = parent;
	}
}

// The out-of-source build root for a repo checkout. This is the single JS/TS encoding of the layout
// formula `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>`; the shell mirror is `buildRoot=$1/$(basename $cmakeDir)`
// (`$1`=`$JDE_BUILD_DIR/$JDE_COMPILER`) in build/buildFunctions.sh (lines 22/74/93/108). A layout change
// must update both — this stays JS (not a shell-out) so the command resolves synchronously and cross-platform.
// Throws if the env is missing rather than yielding a repo-relative path (stray dirs / opaque "program not found").
function repoBuildDir( repoRoot:string ):string {
	const buildDir = process.env['JDE_BUILD_DIR'];
	const compiler = process.env['JDE_COMPILER'];
	const missing = [!buildDir && 'JDE_BUILD_DIR', !compiler && 'JDE_COMPILER'].filter( Boolean );
	if( missing.length ){
		const msg = `jde.repoBuildDir: ${missing.join(' and ')} not set - launch/build paths cannot be resolved. Start VS Code from a shell that sources ~/.profile.`;
		vscode.window.showErrorMessage( msg );
		throw new Error( msg );
	}
	return path.join( buildDir!, compiler!, path.basename(repoRoot) );
}

export function activate(context: vscode.ExtensionContext) {
	context.subscriptions.push( vscode.commands.registerCommand('jde.repoBuildDir', ():string => {
		const folder = vscode.workspace.workspaceFolders?.[0];
		return repoBuildDir( folder ? findRepoRoot(folder.uri.fsPath) : '' );
	}) );
	context.subscriptions.push( vscode.languages.registerDocumentFormattingEditProvider( "cpp", {
		provideDocumentFormattingEdits( document:vscode.TextDocument ):vscode.TextEdit[] {
			console.log('~~Providing document formatting edits for C++');
			let edits:vscode.TextEdit[] = [];
			for( let iLine=0; iLine<document.lineCount; ++iLine )
				edits.push( ...parseLine(document.lineAt(iLine)) );
			return edits;
		}
	}) );
}