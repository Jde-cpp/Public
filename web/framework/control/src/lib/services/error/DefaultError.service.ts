import { Injectable, isDevMode } from '@angular/core';
import {MatSnackBar} from '@angular/material/snack-bar';
import {IErrorService} from './IErrorService'
import { HttpErrorResponse } from '@angular/common/http';
import { Log } from '../IGraphQL'

@Injectable({ providedIn: 'root' })
export class DefaultErrorService implements IErrorService
{
	constructor( private snackbar:MatSnackBar )
	{}

	private showUser( message:string, panelClass:string ){
		this.snackbar.open( message, "", {panelClass: [panelClass], duration: 2000} );
	}
	private showUserError( message:string, log:Log ){
		this.showUser( message ?? "Unknown error", 'red-snackbar' );
		log( message );
	}

	assert( condition:boolean, log:Log ):void{
		if( !condition ) {
			this.showUserError( "assert failed", log );
			throw "assert failed";
		}
	}

	error( error: any, log:Log ){
		//this.showUserError( error && typeof error=='object' ? `${message} - ${error["message"]}` : message, log );
		let message = typeof error=='object' ? error["message"] : error;
		this.showUserError( message, log );
		//this.showUserError( error && typeof error=='object' ? `${message} - ${error["message"]}` : message, log );
	}

	exception( e:any, log:Log ):void{
		if( e instanceof HttpErrorResponse ){
			if( e.error instanceof ProgressEvent )
				this.error( `timeout`, log );  //this.error( `(${e.status})${e.message}`, log );
			else if( e.error && e.error.message )
				this.error( e.error.message, log );
			else
				this.error( `(${e.status})${e.error}`, log );
		}
		else if( typeof e=='object' && typeof e.message=="string" )
			this.showUserError( e.message, log );
		else
			this.showUserError( typeof e=='string' ? e : `Unknown error:  ${JSON.stringify(e)}`, log );//plain-string throws are common in this codebase — must reach the user
	}
	exceptionInfo( e:any, info:string, log:Log ):void{
		if( e instanceof HttpErrorResponse ){
			if( e.error instanceof ProgressEvent )
				this.error( `(${e.status})${e.message}`, log );
			else if( e.error && e.error.message )
				this.error( e.error.message, log );
			else
				this.error( `(${e.status})${e.error}`, log );
		}
		else if( e instanceof Error ){
			this.showUser( `${e.cause}:  ${e.message}`, 'red-snackbar' );
			log( `info: '${info}', cause: '${e.cause}' ${e.stack}` );
		}
		else{
			this.showUser( `${info}  ${typeof e=='string' ? e : JSON.stringify(e)}`, 'red-snackbar' );
			log( `info: '${info}', e: ${JSON.stringify(e)}` );
		}
	}

	warn( message:string ){
		this.showUser( message, 'yellow-snackbar' );
	}

	info( message:string):void{
		this.showUser( message, 'white-snackbar' );
	}
}