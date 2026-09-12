import { HttpErrorResponse } from '@angular/common/http';
import { errorText, httpStatus } from './errors';

//The server's AccessException reaches the browser as a 403 with a PLAIN-TEXT body ("[bob]User does not have 'Read' access
//to 'users'.").  HttpClient asked for json, fails to parse it, and hands the caller {error: SyntaxError, text: body} - which
//errorText printed as "(403)[object Object]".  The body is the text.
describe( 'errorText', ()=>{
	it( 'reads the text out of HttpClient\'s non-json error wrapper', ()=>{
		const e = new HttpErrorResponse( {status: 403, error: {error: new SyntaxError("Unexpected token"), text: "[bob]User does not have 'Read' access to 'users'."}} );
		expect( errorText(e) ).toBe( "(403)[bob]User does not have 'Read' access to 'users'." );
		expect( httpStatus(e) ).toBe( 403 );
	} );
	it( 'still prefers a json body\'s message', ()=>{
		expect( errorText(new HttpErrorResponse({status: 500, error: {message: "boom"}})) ).toBe( "boom" );
	} );
	it( 'prints a string body with its status', ()=>{
		expect( errorText(new HttpErrorResponse({status: 404, error: "gone"})) ).toBe( "(404)gone" );
	} );
} );
