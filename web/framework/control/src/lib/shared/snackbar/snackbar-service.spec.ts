import { MatSnackBar } from '@angular/material/snack-bar';
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

	it('warn shows yellow for 3s and logs', () => {
		const log = vi.spyOn( console, 'warn' ).mockImplementation( () => {} );
		service.warn( 'careful' );
		expect( lastConfig() ).toEqual( {panelClass: ['yellow-snackbar'], data: {message: 'careful', duration: 3000}} );
		expect( log ).toHaveBeenCalledWith( 'careful' );
		log.mockRestore();
	});

	it('info shows white for 3s', () => {
		service.info( 'fyi' );
		expect( lastConfig() ).toEqual( {panelClass: ['white-snackbar'], data: {message: 'fyi', duration: 3000}} );
	});

	it('exception with a plain-string throw reaches the user', () => {
		service.exception( 'string throw', () => {} );
		expect( lastConfig() ).toEqual( {panelClass: ['red-snackbar'], data: {message: 'string throw', duration: 10000}} );
	});

	it('assert(false) shows and throws', () => {
		expect( () => service.assert(false) ).toThrow();
		expect( lastConfig()?.panelClass ).toEqual( ['red-snackbar'] );
	});
});
