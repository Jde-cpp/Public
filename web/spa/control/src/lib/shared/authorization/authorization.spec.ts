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
import { Router } from '@angular/router';
import { signal } from '@angular/core';
import { vi } from 'vitest';
import { EProvider, IAuth, User } from '../../services/authorization/auth';
import { Authorization } from './authorization';

//review3 L8: `gapi` is a `declare const` over index.html's `async defer` platform.js.  A bare `gapi.auth2` is a
//ReferenceError wherever that script has not loaded, and it sat between logout() and the navigate to /login - so the user
//was logged out and left standing on the page they had just lost access to.
describe( 'Authorization.onLogout', ()=>{
	let navigate:any;
	let logout:any;

	const create = ( provider:EProvider )=>{
		navigate = vi.fn().mockResolvedValue( true );
		logout = vi.fn().mockResolvedValue( undefined );
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			{ provide: Router, useValue: {navigate} },
			{ provide: 'IAuth', useValue: {
				user: signal<User|undefined>( {provider} as User ),
				logout,
				providers: ()=>Promise.resolve([])
			} as unknown as IAuth }
		]});
		return TestBed.createComponent( Authorization ).componentInstance;
	};

	afterEach( ()=>{ delete (globalThis as any).gapi; } );

	it( 'still reaches /login when gapi never loaded', async ()=>{
		expect( 'gapi' in globalThis ).toBe( false );//the whole point: the identifier is not merely undefined, it is absent
		await create( EProvider.Google ).onLogout();
		expect( logout ).toHaveBeenCalled();
		expect( navigate ).toHaveBeenCalledWith( ['/login'] );
	} );

	it( 'still reaches /login when signOut rejects', async ()=>{
		(globalThis as any).gapi = { auth2: {getAuthInstance: ()=>({signOut: ()=>Promise.reject(new Error("popup closed"))})} };
		await create( EProvider.Google ).onLogout();
		expect( navigate ).toHaveBeenCalledWith( ['/login'] );
	} );

	it( 'signs out of Google when the library is there', async ()=>{
		const signOut = vi.fn().mockResolvedValue( undefined );
		(globalThis as any).gapi = { auth2: {getAuthInstance: ()=>({signOut})} };
		await create( EProvider.Google ).onLogout();
		expect( signOut ).toHaveBeenCalled();
		expect( navigate ).toHaveBeenCalledWith( ['/login'] );
	} );

	it( 'does not touch Google for a non-Google login', async ()=>{
		const signOut = vi.fn();
		(globalThis as any).gapi = { auth2: {getAuthInstance: ()=>({signOut})} };
		await create( EProvider.OpcServer ).onLogout();
		expect( signOut ).not.toHaveBeenCalled();
		expect( navigate ).toHaveBeenCalledWith( ['/login'] );
	} );
} );
