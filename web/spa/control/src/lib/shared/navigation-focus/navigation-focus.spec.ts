import { Component } from '@angular/core';
import { TestBed } from '@angular/core/testing';
import { vi } from 'vitest';
import { NavigationFocusService } from './navigation-focus-service';
import { NavigationFocus } from './navigation-focus';

//review3 C9: the two @HostBinding decorators moved into the decorator's `host` object.  A render test, because that move
//is not type-checked - a wrong key there sets a literally-named attribute instead of the property it meant to bind.
@Component( {template: '<div focusOnNavigation id="already-set"></div><div focusOnNavigation></div>', imports: [NavigationFocus]} )
class Host{}

describe( 'NavigationFocus host bindings', ()=>{
	let service:any;
	const create = ()=>{
		service = { requestFocusOnNavigation: vi.fn(), requestSkipLinkFocus: vi.fn(), relinquishFocusOnNavigation: vi.fn(), relinquishSkipLinkFocus: vi.fn() };
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [ {provide: NavigationFocusService, useValue: service} ] });
		const fixture = TestBed.createComponent( Host );
		fixture.detectChanges();
		return fixture;
	};

	it( 'makes the host focusable and drops its focus ring', ()=>{
		const el:HTMLElement = create().nativeElement.querySelector( '#already-set' );
		expect( el.tabIndex ).toBe( -1 );//the PROPERTY, which is what @HostBinding('tabindex') set
		expect( el.style.outline ).toBe( 'none' );
	} );

	it( 'registers and unregisters with the focus service', ()=>{
		const fixture = create();
		expect( service.requestFocusOnNavigation ).toHaveBeenCalledTimes( 2 );
		expect( service.requestSkipLinkFocus ).toHaveBeenCalledTimes( 2 );
		fixture.destroy();
		expect( service.relinquishFocusOnNavigation ).toHaveBeenCalledTimes( 2 );
	} );

	it( 'gives an id to a host that has none, and leaves one that has', ()=>{
		const hosts = create().nativeElement.querySelectorAll( '[focusOnNavigation]' );
		expect( hosts[0].id ).toBe( 'already-set' );
		expect( hosts[1].id ).toMatch( /^skip-link-target-\d+$/ );
	} );
} );
