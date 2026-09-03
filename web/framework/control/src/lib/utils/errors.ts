import { HttpErrorResponse } from '@angular/common/http';

//Narrowing for thrown values, so callers can take `unknown` instead of `any` (review3 C4).  Own module rather than
//snackbar-service's, which is where errorText started:  proto-service and the resolvers need this and must not pull
//MatSnackBar in with it.
//
//What the thrown value itself says, in whatever shape it arrived.  undefined when it says nothing quotable (a function,
//an empty object) - the caller's context is then the whole message.
export function errorText( e:unknown ):string|undefined{
	if( e instanceof HttpErrorResponse ){
		if( typeof ProgressEvent!="undefined" && e.error instanceof ProgressEvent )//the global is browser-only:  a bare `instanceof` is a ReferenceError wherever it is not defined, which would replace the error with one of its own
			return "timeout";
		return e.error && e.error.message ? e.error.message : `(${e.status})${e.error}`;
	}
	if( typeof e=='object' && e ){
		const wrapped = (<{error?:{message?:unknown, httpStatus?:unknown}}>e).error;
		if( typeof wrapped?.message=="string" )//a ProtoService rejection - {error:IError}
			return wrapped.httpStatus ? `(${wrapped.httpStatus})${wrapped.message}` : wrapped.message;
		const message = (<{message?:unknown}>e).message;
		if( typeof message=="string" )//Error, and anything else carrying a message
			return message;
	}
	if( typeof e=='string' )//plain-string throws are common in this codebase — must reach the user
		return e;
	let json:string|undefined;
	try{ json = JSON.stringify( e ); }catch{ json = undefined; }//a cyclic throw must not replace the error with a TypeError of its own
	return json && json!="{}" ? `Unknown error:  ${json}` : undefined;
}
//The banner/snackbar text for a throw:  the caller's context, the throw's own words, or both.  Never "[object Object]".
export function errorMessage( e:unknown, info?:string ):string{
	const text = errorText( e );
	return info && text && info!=text ? `${info}  ${text}` : info ?? text ?? "Unknown error";
}
//The http status a throw carries, if any.  Reading `e["status"]` off an `any` is exactly how a typo goes unnoticed.
export function httpStatus( e:unknown ):number|undefined{
	if( e instanceof HttpErrorResponse )
		return e.status;
	const status = typeof e=='object' && e ? (<{status?:unknown}>e).status : undefined;
	return typeof status=="number" ? status : undefined;
}
