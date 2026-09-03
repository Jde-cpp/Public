import { TestBed } from '@angular/core/testing';
import { vi } from 'vitest';
import { ELogLevel } from 'jde-proto/Log';
import { SeverityPicker } from './severity-picker';

//review3 C6: both loops in this template moved from *ngFor to @for.  A render test, because the conversion is only
//compile-checked otherwise - a wrong `track` shows up as duplicated or missing rows, not as a type error.
describe( 'SeverityPicker renders every level', ()=>{
	const create = ( isSelect:boolean )=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({});
		const fixture = TestBed.createComponent( SeverityPicker );
		fixture.componentRef.setInput( 'isSelect', isSelect );
		fixture.componentRef.setInput( 'level', ELogLevel.Warning );
		fixture.detectChanges();
		return fixture;
	};

	it( 'lists one chip per option', ()=>{
		const fixture = create( false );
		const chips = fixture.nativeElement.querySelectorAll( 'mat-chip' );
		expect( chips.length ).toBe( fixture.componentInstance.options.length );
		expect( [...chips].map( (c:Element)=>c.textContent!.trim() ) ).toEqual( ['Trace','Debug','Info','Warning','Error','Critical','None'] );
	} );

	it( 'renders no chips in select mode', ()=>{
		expect( create(true).nativeElement.querySelectorAll('mat-chip').length ).toBe( 0 );//the @if around the listbox
	} );
} );

//review3 C7: `level` became a model().  The old @Input setter emitted levelChange on EVERY write after the first, so a
//parent-driven update bounced straight back at the parent - and the chip path emitted twice, once from the setter and
//once from onSelectionChange.
describe( 'SeverityPicker.level emits once, and only for the user', ()=>{
	const create = ()=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({});
		const fixture = TestBed.createComponent( SeverityPicker );
		fixture.componentRef.setInput( 'level', ELogLevel.Warning );
		fixture.detectChanges();
		const emitted:ELogLevel[] = [];
		fixture.componentInstance.level.subscribe( (l:ELogLevel)=>emitted.push(l) );
		return { fixture, emitted };
	};

	it( 'emits exactly once when the user picks a level', ()=>{
		const { fixture, emitted } = create();
		fixture.componentInstance.onSelectionChange( ELogLevel.Error );
		expect( emitted ).toEqual( [ELogLevel.Error] );
	} );

	it( 'does not emit when the parent writes the input', ()=>{
		const { fixture, emitted } = create();
		fixture.componentRef.setInput( 'level', ELogLevel.Critical );
		fixture.detectChanges();
		expect( emitted ).toEqual( [] );//the bounce-back the old setter produced
		expect( fixture.componentInstance.level() ).toBe( ELogLevel.Critical );
	} );
} );
