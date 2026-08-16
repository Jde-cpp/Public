//ProfileStore server persistence: logged in it loads/saves through the 'IProfileService' token with a 10-entry LRU of
//JSON strings (parse-per-load so callers never share mutable objects); logged out or on failure it falls back to
//localStorage exactly as the pre-server implementation did.

//the vitest environment provides window/document but no localStorage - back the bare-global references with an
//in-memory one BEFORE importing jde-spa (see google-relogin.spec.ts).
if( typeof globalThis.localStorage=="undefined" ){
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

import { signal } from '@angular/core';
import { IProfileService, ProfileStore } from 'jde-spa';

class MockProfileService implements IProfileService{
	constructor( user:string|undefined='u1' ){ this.#user.set( user ); }
	#user = signal<string|undefined>( undefined );
	userKey = this.#user.asReadonly();
	setUser( u:string|undefined ){ this.#user.set( u ); }
	rows = new Map<string,string>();
	load = vi.fn( async ( key:string )=>this.rows.has(key) ? this.rows.get(key)! : null );
	save = vi.fn( async ( key:string, value:string|null )=>{ value==null ? this.rows.delete(key) : this.rows.set(key, value); } );
}

describe( 'ProfileStore', ()=>{
	beforeEach( ()=>localStorage.clear() );

	it( 'logged out: load/save stay on localStorage and never touch the server', async ()=>{
		const service = new MockProfileService();
		service.setUser( undefined );//logged out
		const store = new ProfileStore( service );
		localStorage.setItem( 'k', JSON.stringify({a:1}) );
		await expect( store.load('k', {a:0}) ).resolves.toEqual( {a:1} );
		await store.save( 'k', {a:2} );
		expect( localStorage.getItem('k') ).toBe( JSON.stringify({a:2}) );
		expect( service.load ).not.toHaveBeenCalled();
		expect( service.save ).not.toHaveBeenCalled();
	});

	it( 'no provider: pure localStorage', async ()=>{
		const store = new ProfileStore( null );
		await expect( store.load('k', 'd') ).resolves.toBe( 'd' );
		await store.save( 'k', 'v' );
		expect( localStorage.getItem('k') ).toBe( 'v' );
	});

	it( 'logged in: one fetch per key, later loads served from the cache with a fresh parse each time', async ()=>{
		const service = new MockProfileService();
		service.rows.set( 'k', JSON.stringify({a:1}) );
		const store = new ProfileStore( service );
		const first = await store.load( 'k', {} );
		const second = await store.load( 'k', {} );
		expect( service.load ).toHaveBeenCalledTimes( 1 );
		expect( second ).toEqual( first );
		expect( second ).not.toBe( first );//callers must not share one mutable object
	});

	it( 'no server row + local value: migrates the local value up', async ()=>{
		const service = new MockProfileService();
		const store = new ProfileStore( service );
		localStorage.setItem( 'k', JSON.stringify(['local']) );
		await expect( store.load('k', []) ).resolves.toEqual( ['local'] );
		await vi.waitFor( ()=>expect(service.save).toHaveBeenCalledWith('k', JSON.stringify(['local'])) );
	});

	it( 'no server row + no local value: default returned and the absence is cached', async ()=>{
		const service = new MockProfileService();
		const store = new ProfileStore( service );
		await expect( store.load('k', 'd') ).resolves.toBe( 'd' );
		await expect( store.load('k', 'd2') ).resolves.toBe( 'd2' );
		expect( service.load ).toHaveBeenCalledTimes( 1 );//negative-cached
	});

	it( 'save dual-writes localStorage and skips the mutation when the value is unchanged', async ()=>{
		const service = new MockProfileService();
		const store = new ProfileStore( service );
		await store.save( 'k', {a:1} );
		await store.save( 'k', {a:1} );//unchanged ngOnDestroy-style save
		expect( localStorage.getItem('k') ).toBe( JSON.stringify({a:1}) );
		expect( service.save ).toHaveBeenCalledTimes( 1 );
		await store.save( 'k', {a:2} );
		expect( service.save ).toHaveBeenCalledTimes( 2 );
	});

	it( 'load failure falls back to localStorage and is not cached', async ()=>{
		const service = new MockProfileService();
		service.load.mockRejectedValue( new Error('down') );
		const store = new ProfileStore( service );
		localStorage.setItem( 'k', JSON.stringify('local') );
		const warn = vi.spyOn( console, 'warn' ).mockImplementation( ()=>{} );
		try{
			await expect( store.load('k', 'd') ).resolves.toBe( 'local' );
			await store.load( 'k', 'd' );
			expect( service.load ).toHaveBeenCalledTimes( 2 );//failure retried, not cached
		}
		finally{ warn.mockRestore(); }
	});

	it( 'save failure keeps the localStorage copy', async ()=>{
		const service = new MockProfileService();
		service.save.mockRejectedValue( new Error('down') );
		const store = new ProfileStore( service );
		const warn = vi.spyOn( console, 'warn' ).mockImplementation( ()=>{} );
		try{
			await store.save( 'k', 'v' );
			expect( localStorage.getItem('k') ).toBe( 'v' );
		}
		finally{ warn.mockRestore(); }
	});

	it( 'the 11th key evicts the least-recently-used entry', async ()=>{
		const service = new MockProfileService();
		const store = new ProfileStore( service );
		for( let i=0; i<11; ++i )
			await store.load( `k${i}`, 'd' );
		expect( service.load ).toHaveBeenCalledTimes( 11 );
		await store.load( 'k10', 'd' );//still cached
		expect( service.load ).toHaveBeenCalledTimes( 11 );
		await store.load( 'k0', 'd' );//evicted ⇒ refetched
		expect( service.load ).toHaveBeenCalledTimes( 12 );
	});

	it( 'cache entries are user-scoped: a different login cannot hit them', async ()=>{
		const service = new MockProfileService();
		service.rows.set( 'k', JSON.stringify('a-data') );
		const store = new ProfileStore( service );
		await expect( store.load('k', 'd') ).resolves.toBe( 'a-data' );
		service.setUser( 'u2' );
		service.rows.set( 'k', JSON.stringify('b-data') );
		await expect( store.load('k', 'd') ).resolves.toBe( 'b-data' );
		expect( service.load ).toHaveBeenCalledTimes( 2 );
	});

	it( 'loadClassArray rehydrates through the constructor with extra args', async ()=>{
		class Item{
			constructor( public json:{n:number}, public extra:string ){}
		}
		const service = new MockProfileService();
		service.rows.set( 'k', JSON.stringify([{n:1},{n:2}]) );
		const store = new ProfileStore( service );
		const items = await store.loadClassArray( 'k', Item, 'x' );
		expect( items ).toHaveLength( 2 );
		expect( items[0] ).toBeInstanceOf( Item );
		expect( items[1].json.n ).toBe( 2 );
		expect( items[0].extra ).toBe( 'x' );
	});
});
