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

//The workspace's `buildDir` setting, undefined when unset or blank. It is the workspace file's own un-namespaced
//key - it sits next to `sourceDir`/`target`/`presetSuffix` in the .code-workspace rather than being contributed by
//this extension - because the configuration model keeps unregistered keys, which is also what `${config:...}` in
//tasks.json reads, so a workspace spells the value once and uses it from both places.
function buildDirSetting():string|undefined {
	const value = vscode.workspace.getConfiguration( undefined, vscode.workspace.workspaceFolders?.[0]?.uri ).get<string>( 'buildDir' )?.trim();
	return value?.length ? value : undefined;
}

//A setting read through the configuration API comes back verbatim: VS Code only expands ${env:...} and
//${workspaceFolder} in tasks.json/launch.json, so do it here for the two forms the workspace files use.
//An unset variable throws rather than collapsing to '' - same reason repoBuildDirFrom throws below, a
//truncated root silently builds into the wrong tree.
function expandVars( value:string, repoRoot:string, source:string ):string {
	return value.replace( /\$\{(?:env:(\w+)|(workspaceFolder))\}/g, ( match:string, envName:string|undefined ):string => {
		if( !envName )
			return repoRoot;
		const expanded = process.env[envName];
		if( !expanded ){
			const msg = `${source}: '${match}' is not set - launch/build paths cannot be resolved. Start VS Code from a shell that sources ~/.profile.`;
			vscode.window.showErrorMessage( msg );
			throw new Error( msg );
		}
		return expanded;
	});
}

// The out-of-source build root for a repo checkout. This is the single encoding of the layout
// formula `$JDE_BUILD_DIR/$JDE_COMPILER/<repo-basename>` (without the trailing `<debug|release>` segment).
// build/buildFunctions.sh no longer re-derives this: its helpers now receive the full build dir - callers
// compose `${command:jde.repoBuildDir}/<debug|release>` - and read the source dir back from CMakeCache.txt.
// This stays JS (not a shell-out) so the command resolves synchronously and cross-platform.
// Throws if the env is missing rather than yielding a repo-relative path (stray dirs / opaque "program not found").
//The `buildDir` setting wins over `vars`, the build-root env vars in precedence order (the first one set wins).
//The setting is the root *verbatim* - only the env formula appends $JDE_COMPILER/<repo>. That is the point of it:
//repos.code-workspace's dependency tree is a flat `$JDE_DEPENDS_BUILD_DIR/<debug|release>` with neither segment,
//which the formula cannot express. Both commands read the same setting, so pinning it puts debug and release under
//one root - the release/debug split is a $JDE_RBUILD_DIR concern, and a workspace that pins the root has none.
//Relative values resolve against the checkout; path.resolve also collapses the `../..` such a value carries.
function repoBuildDirFrom( repoRoot:string, command:string, ...vars:string[] ):string {
	const setting = buildDirSetting();
	let joined:string;
	if( setting )
		joined = path.resolve( repoRoot, expandVars(setting, repoRoot, `${command}: setting 'buildDir'`) );
	else{
		const buildDir = vars.map( v=>process.env[v] ).find( Boolean );
		const compiler = process.env['JDE_COMPILER'];
		const missing = [!buildDir && `${vars.join(' or ')} (nor the buildDir setting)`, !compiler && 'JDE_COMPILER'].filter( Boolean );
		if( missing.length ){
			const msg = `${command}: ${missing.join(' and ')} not set - launch/build paths cannot be resolved. Start VS Code from a shell that sources ~/.profile.`;
			vscode.window.showErrorMessage( msg );
			throw new Error( msg );
		}
		joined = path.join( buildDir!, compiler!, path.basename(repoRoot) );
	}
	//tasks.json splices this into an unquoted bash command line (see cmakeDebug), which VS Code tokenizes
	//and bash then parses again - backslashes get eaten as escape chars either way. '/' has no escaping
	//meaning to either layer and is a valid Windows path separator, so use it instead of trying to survive escaping.
	return process.platform == 'win32' ? joined.replace( /\\/g, '/' ) : joined;
}

function repoBuildDir( repoRoot:string ):string {
	return repoBuildDirFrom( repoRoot, 'jde.repoBuildDir', 'JDE_BUILD_DIR' );
}

// The same layout rooted at $JDE_RBUILD_DIR when it is set, so a checkout can put its release outputs on a
// different volume than the debug tree. Falls back to $JDE_BUILD_DIR, so a machine that never sets
// JDE_RBUILD_DIR resolves identically to repoBuildDir - as does a workspace that sets `buildDir`.
function repoBuildRelDir( repoRoot:string ):string {
	return repoBuildDirFrom( repoRoot, 'jde.repoBuildRelDir', 'JDE_RBUILD_DIR', 'JDE_BUILD_DIR' );
}

export function activate(context: vscode.ExtensionContext) {
	const workspaceRepoRoot = ():string => {
		const folder = vscode.workspace.workspaceFolders?.[0];
		return folder ? findRepoRoot( folder.uri.fsPath ) : '';
	};
	context.subscriptions.push( vscode.commands.registerCommand('jde.repoBuildDir', ():string => repoBuildDir(workspaceRepoRoot())) );
	context.subscriptions.push( vscode.commands.registerCommand('jde.repoBuildRelDir', ():string => repoBuildRelDir(workspaceRepoRoot())) );
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