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
			duration = duration ?? 3000;
			break;
		case Type.Error:
			duration = duration ?? 5000;
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
	private processEvent( e:any, type:Type=Type.Error, info?:string ):void{
		if( e instanceof HttpErrorResponse ){
			if( e.error instanceof ProgressEvent )
				this.show( `timeout`, type, e );
			else if( e.error && e.error.message )
				this.show( e.error.message, type, e );
			else
				this.show( `(${e.status})${e.error}`, type, e );
		}
		else if( typeof e=='object' && typeof e?.error?.message=="string" )//a ProtoService rejection - {error:IError}; was falling through to the JSON.stringify fallback
			this.show( e.error.httpStatus ? `(${e.error.httpStatus})${e.error.message}` : e.error.message, type, e );
		else if( typeof e=='object' && typeof e.message=="string" )
			this.show( e.message, type, e );
		else if( e instanceof Error )
			this.show( `${e.cause}:  ${e.message}`, Type.Error, e );
		else if( info )
			this.show( `${info}  ${typeof e=='string' ? e : JSON.stringify(e)}`, Type.Error, e );
		else
			this.show( typeof e=='string' ? e : `Unknown error:  ${JSON.stringify(e)}`, type, e );//plain-string throws are common in this codebase — must reach the user
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
