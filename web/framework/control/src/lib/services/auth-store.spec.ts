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
import { RouteItem, RouteStore } from 'jde-spa';
import { AuthStore } from './auth-store';

//angular-review3 L3: RouteStore caches the NAMES of the rows the last user browsed, and nothing dropped them at logout.
//AuthStore.logout is the funnel - the navbar button reaches it through AppService/GatewayService, and so do both 401
//handlers once silent renewal has given up.
describe( 'AuthStore.logout clears the browsed route names', ()=>{
	beforeEach( ()=>{
		localStorage.clear();
		TestBed.resetTestingModule();
	} );

	it( 'leaves no cached children behind', ()=>{
		const routeStore = TestBed.inject( RouteStore );
		routeStore.setChildren( 'users', [new RouteItem({title:'Alice', path:'alice', icon:''})] );
		expect( localStorage.getItem('users') ).not.toBeNull();

		TestBed.inject( AuthStore ).logout();

		expect( localStorage.getItem('users') ).toBeNull();
		expect( routeStore.entries().size ).toBe( 0 );
	} );
} );
