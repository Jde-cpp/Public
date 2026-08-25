import { Injectable } from '@angular/core';
import {MatSnackBar} from '@angular/material/snack-bar';
import { HttpErrorResponse } from '@angular/common/http';
import { Snackbar } from './snackbar';

enum Type{
	Info = 'blue-snackbar',
	Warning =  'yellow-snackbar',
	Error = 'red-snackbar'
}
@Injectable({ providedIn: 'root' })
export class SnackbarService{
	constructor( private snackbar:MatSnackBar )
	{}

	//DevTools anchors these console lines at this file, not at the caller.  To get the caller back, ignore-list it:  right-click the snackbar-service.ts frame in the console → "Add script to ignore list", or DevTools Settings → Ignore list → add a pattern.  Working around this is what the old `log:Log` parameter existed for.
	private static write( type:Type, message:string, detail?:any ):void{
		const args = detail===undefined || detail===message ? [message] : [message, detail];//log the throw itself, not its text:  DevTools renders an Error's stack expandable.
		switch( type ){
		case Type.Info:
			console.log( ...args );
			break;
		case Type.Warning:
			console.warn( ...args );
			break;
		case Type.Error:
			console.error( ...args );
			break;
		}
	}

	private show( userMessage:string, type:Type, detail?:any, duration?:number ){
		switch( type ){
		case Type.Info:
			duration = duration ?? 1000;
			break;
		case Type.Warning:
			duration = duration ?? 5000;
			break;
		case Type.Error:
			duration = duration ?? 10000;
			break;
		}
		SnackbarService.write( type, userMessage ?? "Unknown error", detail );
		this.snackbar.openFromComponent( Snackbar, {panelClass: [type], data: {message: userMessage ?? "Unknown error", duration}} );
	}
	private showError( userMessage:string|undefined, detail:any ){
		this.show( userMessage ?? "Unknown error", Type.Error, detail );
	}

	assert( condition:boolean ):void{
		if( !condition ) {
			this.showError( "assert failed", "assert failed" );
			throw "assert failed";
		}
	}
	//What the thrown value itself says, in whatever shape it arrived.  undefined when it says nothing quotable (a function,
	//an empty object) - the caller's context is then the whole message.
	private static errorText( e:any ):string|undefined{
		if( e instanceof HttpErrorResponse ){
			if( typeof ProgressEvent!="undefined" && e.error instanceof ProgressEvent )//the global is browser-only:  a bare `instanceof` is a ReferenceError wherever it is not defined, which would replace the error with one of its own
				return "timeout";
			return e.error && e.error.message ? e.error.message : `(${e.status})${e.error}`;
		}
		if( typeof e=='object' && e ){
			if( typeof e.error?.message=="string" )//a ProtoService rejection - {error:IError}
				return e.error.httpStatus ? `(${e.error.httpStatus})${e.error.message}` : e.error.message;
			if( typeof e.message=="string" )//Error, and anything else carrying a message
				return e.message;
		}
		if( typeof e=='string' )//plain-string throws are common in this codebase — must reach the user
			return e;
		let json:string|undefined;
		try{ json = JSON.stringify( e ); }catch{ json = undefined; }//a cyclic throw must not replace the error with a TypeError of its own
		return json && json!="{}" ? `Unknown error:  ${json}` : undefined;
	}
	//`info` is the caller's context ("Save failed.", "Could not load 'readers'").  It used to be dropped for every shape but
	//the last two, which meant the two that actually reach production - HttpErrorResponse and a {error:IError} rejection -
	//showed the bare server text with no clue which page or action produced it.  It prefixes the message now.
	private processEvent( e:any, type:Type=Type.Error, info?:string ):void{
		const text = SnackbarService.errorText( e );
		const message = info && text && info!=text ? `${info}  ${text}` : info ?? text ?? "Unknown error";
		this.show( message, type, e );
	}

	exception( message:string, e:any ){
		this.processEvent( e, Type.Error, message );
	}

	error( message:string ){
		this.processEvent( message, Type.Error );
	}

	warn( message:string|any ){
		this.processEvent( message, Type.Warning );
	}

	info( message:string|any ):void{
		this.processEvent( message, Type.Info );
	}
}
