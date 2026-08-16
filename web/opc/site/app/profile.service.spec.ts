//ProfileService (jde-framework): the 'IProfileService' implementation ProfileStore persists through - exact QL
//strings against a mocked AppService, and the logged-in predicate behind userKey.

if( typeof globalThis.localStorage=="undefined" ){//AppService's import chain touches localStorage at module scope
	const backing = new Map<string,string>();
	const storage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>{ backing.clear(); },
	};
	(globalThis as any).localStorage = storage;
	(window as any).localStorage = storage;
}

import { TestBed } from '@angular/core/testing';
import { signal } from '@angular/core';
import { AppService, ProfileService } from 'jde-framework';
import { User } from 'jde-spa';

describe( 'ProfileService', ()=>{
	const user = signal<User|undefined>( undefined );
	let app:{ user:typeof user, querySingle:any, mutate:any };
	let service:ProfileService;

	beforeEach( ()=>{
		user.set( undefined );
		app = { user, querySingle: vi.fn(), mutate: vi.fn() };
		TestBed.configureTestingModule( { providers: [{provide: AppService, useValue: app}, ProfileService] } );
		service = TestBed.inject( ProfileService );
	});
	afterEach( ()=>TestBed.resetTestingModule() );

	it( 'userKey is undefined until a jwt or id marks the user logged in', ()=>{
		expect( service.userKey() ).toBeUndefined();
		user.set( {sessionId:'anon'} as User );//anonymous REST session: not logged in
		expect( service.userKey() ).toBeUndefined();
		user.set( {id:'bob'} as User );
		expect( service.userKey() ).toBe( 'bob' );
		user.set( {jwt:'j', email:'g@x.com'} as User );//Google login without an id
		expect( service.userKey() ).toBe( 'g@x.com' );
	});

	it( 'load queries profile by target and unwraps value', async ()=>{
		app.querySingle.mockResolvedValue( {value:'{"a":1}'} );
		await expect( service.load('qlList/users/views') ).resolves.toBe( '{"a":1}' );
		expect( app.querySingle ).toHaveBeenCalledWith( 'profile( target:"qlList/users/views" ){ value }' );
	});

	it( 'load returns null when the server has no row', async ()=>{
		app.querySingle.mockResolvedValue( null );
		await expect( service.load('k') ).resolves.toBeNull();
	});

	it( 'save mutates updateProfile with the escaped value', async ()=>{
		app.mutate.mockResolvedValue( {} );
		await service.save( 'favorites', '[{"name":"a b"}]' );
		expect( app.mutate ).toHaveBeenCalledTimes( 1 );
		expect( app.mutate.mock.calls[0][0].toString() ).toBe( 'updateProfile( target:"favorites", value:"[{\\"name\\":\\"a b\\"}]" )' );
	});

	it( 'save with null deletes the row', async ()=>{
		app.mutate.mockResolvedValue( {} );
		await service.save( 'k', null );
		expect( app.mutate.mock.calls[0][0].toString() ).toBe( 'updateProfile( target:"k", value:null )' );
	});
});
