import { TestBed } from '@angular/core/testing';
import { MatSnackBar } from '@angular/material/snack-bar';
import { HttpErrorResponse } from '@angular/common/http';
import { SnackbarService } from './snackbar-service';
import { errorMessage, errorText, httpStatus } from '../../utils/errors';
import { Snackbar } from './snackbar';

describe('SnackbarService', () => {
	const snackbar = { openFromComponent: vi.fn() };
	//MatSnackBar is inject()ed since C5, so the stub goes in through the injector rather than the constructor
	TestBed.configureTestingModule({ providers: [ {provide: MatSnackBar, useValue: snackbar} ] });
	const service = TestBed.inject( SnackbarService );
	const lastConfig = () => snackbar.openFromComponent.mock.lastCall?.[1];

	beforeEach( () => snackbar.openFromComponent.mockClear() );

	it('error shows red for 10s and logs', () => {
		const log = vi.spyOn( console, 'error' ).mockImplementation( () => {} );
		service.error( 'boom' );
		expect( snackbar.openFromComponent ).toHaveBeenCalledWith( Snackbar, {panelClass: ['red-snackbar'], data: {message: 'boom', duration: 10000}} );
		expect( log ).toHaveBeenCalledWith( 'boom' );
		log.mockRestore();
	});

	it('warn shows yellow for 5s and logs', () => {
		const log = vi.spyOn( console, 'warn' ).mockImplementation( () => {} );
		service.warn( 'careful' );
		expect( lastConfig() ).toEqual( {panelClass: ['yellow-snackbar'], data: {message: 'careful', duration: 5000}} );
		expect( log ).toHaveBeenCalledWith( 'careful' );
		log.mockRestore();
	});

	it('info shows blue for 1s', () => {
		service.info( 'fyi' );
		expect( lastConfig() ).toEqual( {panelClass: ['blue-snackbar'], data: {message: 'fyi', duration: 1000}} );
	});

	it('a throw with nothing quotable leaves the context as the whole message', () => {
		service.exception( 'string throw', () => {} );
		expect( lastConfig() ).toEqual( {panelClass: ['red-snackbar'], data: {message: 'string throw', duration: 10000}} );
	});

	it('a ProtoService rejection shows status and message, not JSON', () => {
		service.exception( 'ctx', {error: {requestId: 5, message: 'denied', sc: 3, httpStatus: 401}} );
		expect( lastConfig()?.data.message ).toBe( 'ctx  (401)denied' );
		service.exception( 'ctx', {error: {message: 'boom'}} );//no status - message alone, no "(undefined)".
		expect( lastConfig()?.data.message ).toBe( 'ctx  boom' );
	});

	//the caller's context used to be dropped for exactly the shapes that reach production, so a 500 surfaced as the bare
	//server text with no clue which page produced it.
	it('an HttpErrorResponse keeps the caller context and the server text', () => {
		service.exception( "Could not load 'readers'", new HttpErrorResponse({status: 500, error: 'Query failed.'}) );
		expect( lastConfig()?.data.message ).toBe( "Could not load 'readers'  (500)Query failed." );
	});

	it('a context equal to the error text is not doubled', () => {
		service.exception( 'boom', 'boom' );
		expect( lastConfig()?.data.message ).toBe( 'boom' );
	});

	it('assert(false) shows and throws', () => {
		expect( () => service.assert(false) ).toThrow();
		expect( lastConfig()?.panelClass ).toEqual( ['red-snackbar'] );
	});
});

//review3 L13: the inline error banners built their text with `${e}`, which renders "[object Object]" for the two shapes
//that actually reach production - an HttpErrorResponse and a {error:IError} ProtoService rejection.  errorText was the
//snackbar's private static; it is module scope now so a banner and a snackbar cannot disagree about the same throw.
describe( 'errorMessage', ()=>{
	const rejection = { error: {requestId: 1, message: "no such instance", sc: 0, httpStatus: 404} };
	const http = new HttpErrorResponse( {status: 500, error: {message: "server exploded"}} );

	it( 'never renders [object Object]', ()=>{
		for( const e of [rejection, http, new Error("boom"), "plain string", {}, undefined] )
			expect( errorMessage(e, "Could not load.") ).not.toContain( "[object Object]" );
	} );

	it( 'quotes a ProtoService rejection with its http status', ()=>{
		expect( errorMessage(rejection, "Could not load.") ).toBe( "Could not load.  (404)no such instance" );
	} );

	it( 'quotes an HttpErrorResponse', ()=>{
		expect( errorMessage(http, "Could not load.") ).toBe( "Could not load.  server exploded" );
	} );

	it( "falls back to the caller's context when the throw says nothing quotable", ()=>{
		expect( errorMessage({}, "Could not load.") ).toBe( "Could not load." );
		expect( errorText({}) ).toBeUndefined();
	} );

	it( 'says something even with no context at all', ()=>{
		expect( errorMessage(undefined) ).toBe( "Unknown error" );
	} );
} );

//review3 C4: handle401 used to read `e["status"]` off an `any`, which compiled whatever it was spelled.  This narrows once.
describe( 'httpStatus', ()=>{
	it( 'reads an HttpErrorResponse', ()=>{
		expect( httpStatus(new HttpErrorResponse({status: 401})) ).toBe( 401 );
	} );
	it( 'reads a plain rejection carrying a numeric status', ()=>{
		expect( httpStatus({status: 0, error: 'net'}) ).toBe( 0 );//0 is a real status here - the opaque network failure - so it must not read as absent
	} );
	it( 'is undefined for anything else', ()=>{
		for( const e of [new Error("boom"), "plain", {status: "401"}, undefined, null] )
			expect( httpStatus(e) ).toBeUndefined();
	} );
} );
