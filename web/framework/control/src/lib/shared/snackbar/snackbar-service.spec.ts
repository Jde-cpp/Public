import { MatSnackBar } from '@angular/material/snack-bar';
import { HttpErrorResponse } from '@angular/common/http';
import { SnackbarService } from './snackbar-service';
import { Snackbar } from './snackbar';

describe('SnackbarService', () => {
	const snackbar = { openFromComponent: vi.fn() };
	const service = new SnackbarService( snackbar as unknown as MatSnackBar );
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
