//the vitest environment provides window/document but no localStorage - back the bare-global references with an
//in-memory one BEFORE importing jde-spa (see google-relogin.spec.ts).
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
import { lastValueFrom } from 'rxjs';
import { ISearchProvider, SEARCH_PROVIDERS, SearchResult, SearchService } from 'jde-spa';

const hit = ( title:string, source:string, rank=0, prefix?:string ):SearchResult=>({ title, route: `/${source}/${title}`, rank, source, prefix });

function provider( name:string, results:SearchResult[]|Error, prefixes?:string[] ):ISearchProvider & { calls:{text:string, scope?:string}[] }{
	return {
		name, prefixes, calls: [],
		async search( text, scope ){ this.calls.push( {text, scope} ); if( results instanceof Error ) throw results; return results; }
	};
}

describe('SearchService', () => {
	let routes:ReturnType<typeof provider>, access:ReturnType<typeof provider>, broken:ReturnType<typeof provider>;
	let service:SearchService;
	beforeEach( () => {
		routes = provider( 'routes', [hit('Gateways', 'routes', 0), hit('Access', 'routes', 1)] );
		access = provider( 'access', [hit('alice', 'access', 0, 'user'), {...hit('Gateways', 'routes', 0), source: 'access'}], ['user','role'] );//the second hit duplicates a route hit (same route).
		broken = provider( 'broken', new Error('down') );
		TestBed.configureTestingModule({ providers: [
			{ provide: SEARCH_PROVIDERS, useValue: routes, multi: true },
			{ provide: SEARCH_PROVIDERS, useValue: access, multi: true },
			{ provide: SEARCH_PROVIDERS, useValue: broken, multi: true },
		]});
		service = TestBed.inject( SearchService );
	});

	it('parses a registered prefix into a scope and leaves other colons as text', () => {
		expect( service.parse('user:Al') ).toEqual( {scope: 'user', text: 'al'} );
		expect( service.parse('  role:  ') ).toEqual( {scope: 'role', text: ''} );
		expect( service.parse('foo:bar') ).toEqual( {text: 'foo:bar'} );
		expect( service.parse('Pump') ).toEqual( {text: 'pump'} );
	});

	it('merges every provider, dedupes on route, ranks then keeps registration order, and survives a failing provider', async () => {
		const warn = vi.spyOn( console, 'warn' ).mockImplementation( ()=>{} );
		const results = await lastValueFrom( service.search('a') );
		expect( results.map(r=>`${r.source}:${r.title}`) ).toEqual( ['routes:Gateways', 'access:alice', 'routes:Access'] );
		expect( warn ).toHaveBeenCalled();
		expect( broken.calls ).toHaveLength( 1 );
		warn.mockRestore();
	});

	it('a scope only asks the providers that list it', async () => {
		vi.spyOn( console, 'warn' ).mockImplementation( ()=>{} );
		const results = await lastValueFrom( service.search('user:al', 5) );
		expect( routes.calls ).toHaveLength( 0 );
		expect( access.calls ).toEqual( [{text: 'al', scope: 'user'}] );
		expect( results.map(r=>r.title) ).toEqual( ['alice', 'Gateways'] );
	});

	it('caps at the limit and returns nothing for blank text', async () => {
		vi.spyOn( console, 'warn' ).mockImplementation( ()=>{} );
		expect( await lastValueFrom(service.search('a', 1)) ).toHaveLength( 1 );
		expect( await lastValueFrom(service.search('   ')) ).toEqual( [] );
	});
});
