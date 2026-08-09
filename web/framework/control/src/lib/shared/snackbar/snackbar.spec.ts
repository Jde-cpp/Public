import { TestBed } from '@angular/core/testing';
import { MAT_SNACK_BAR_DATA, MatSnackBarRef } from '@angular/material/snack-bar';
import { Snackbar } from './snackbar';

describe('Snackbar', () => {
	const ref = { dismiss: vi.fn() };

	beforeEach( async () => {
		vi.useFakeTimers();
		ref.dismiss.mockClear();
		await TestBed.configureTestingModule({
			imports: [Snackbar],
			providers: [
				{provide: MatSnackBarRef, useValue: ref},
				{provide: MAT_SNACK_BAR_DATA, useValue: {message: 'boom', duration: 1000}}
			]
		}).compileComponents();
	});
	afterEach( () => vi.useRealTimers() );

	it('auto-dismisses after duration', () => {
		TestBed.createComponent( Snackbar );
		vi.advanceTimersByTime( 1001 );
		expect( ref.dismiss ).toHaveBeenCalled();
	});

	it('pin cancels auto-dismiss', () => {
		const fixture = TestBed.createComponent( Snackbar );
		fixture.componentInstance.togglePin();
		vi.advanceTimersByTime( 5000 );
		expect( ref.dismiss ).not.toHaveBeenCalled();
	});

	it('unpin restarts the full countdown', () => {
		const fixture = TestBed.createComponent( Snackbar );
		fixture.componentInstance.togglePin();
		vi.advanceTimersByTime( 5000 );
		fixture.componentInstance.togglePin();
		vi.advanceTimersByTime( 999 );
		expect( ref.dismiss ).not.toHaveBeenCalled();
		vi.advanceTimersByTime( 2 );
		expect( ref.dismiss ).toHaveBeenCalled();
	});

	it('close dismisses immediately', () => {
		const fixture = TestBed.createComponent( Snackbar );
		fixture.componentInstance.ref.dismiss();
		expect( ref.dismiss ).toHaveBeenCalled();
	});

	it('destroy clears the pending timer', () => {
		const fixture = TestBed.createComponent( Snackbar );
		fixture.destroy();
		vi.advanceTimersByTime( 5000 );
		expect( ref.dismiss ).not.toHaveBeenCalled();
	});
});
