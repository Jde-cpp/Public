if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { TestBed } from '@angular/core/testing';
import { provideRouter, Router } from '@angular/router';
import { signal } from '@angular/core';
import { vi } from 'vitest';
import { NavigationFocusService } from '../navigation-focus/navigation-focus-service';
import { SearchService } from '../../services/search/search-service';
import { SearchResult } from '../../services/search/search-provider';
import { NavBar } from './navbar';

//review3 L9: onSearch called event.preventDefault() and THEN tested event.defaultPrevented - the flag it had just set - so
//the Enter fallback below it was unreachable and Enter with nothing highlighted did nothing.
describe( 'NavBar.onSearch', ()=>{
	const first:SearchResult = {title:'Alice', route:'/access/users/alice'} as SearchResult;

	//NavBar's searchTrigger/searchInput are viewChild() and searchResults is a read-only toSignal, so a rendered fixture
	//would need the whole autocomplete; the handler under test reads only the four members overridden here.
	class TestNavBar extends NavBar{
		override searchTrigger = signal<any>( undefined );
		override searchResults = signal<SearchResult[]>( [first] );
		override searchInput = signal<any>( {nativeElement: {blur: ()=>{}}} );
		selected:SearchResult[] = [];
		override onSearchSelected( result:SearchResult ){ this.selected.push( result ); }
	}

	const create = ()=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			provideRouter( [{path: 'x', children: [], title: 'X'}] ),
			{ provide: NavigationFocusService, useValue: {} },
			{ provide: SearchService, useValue: {search: ()=>Promise.resolve([])} }
		]});
		return TestBed.runInInjectionContext( ()=>new TestNavBar() );//NavigationFocusService is inject()ed since C5
	};

	const enter = ()=>new KeyboardEvent( 'keydown', {key:'Enter', cancelable:true} );

	it( 'navigates to the first result when nothing is highlighted', ()=>{
		const navbar = create();
		const event = enter();
		navbar.onSearch( event );
		expect( navbar.selected ).toEqual( [first] );
		expect( event.defaultPrevented ).toBe( true );//still swallowed - the input must never submit
	} );

	it( 'leaves the highlighted option to the autocomplete', ()=>{
		const navbar = create();
		navbar.searchTrigger.set( {panelOpen: true, activeOption: {}} );
		navbar.onSearch( enter() );
		expect( navbar.selected ).toEqual( [] );
	} );

	it( 'honours an event another handler already consumed', ()=>{
		const navbar = create();
		const event = enter();
		event.preventDefault();//as the autocomplete trigger does once it has selected the active option
		navbar.onSearch( event );
		expect( navbar.selected ).toEqual( [] );
	} );

	it( 'does nothing when there are no results', ()=>{
		const navbar = create();
		navbar.searchResults.set( [] );
		navbar.onSearch( enter() );
		expect( navbar.selected ).toEqual( [] );
	} );
} );
