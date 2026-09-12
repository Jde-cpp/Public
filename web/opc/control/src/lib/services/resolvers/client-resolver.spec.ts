if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { vi } from 'vitest';
import { DetailRoute, SnackbarService, SlugNotFoundError } from 'jde-framework';
import { Gateway } from '../gateway-service';
import { OpcStore } from '../opc-store';
import { ClientResolver } from './client-resolver';

//review3 L14: ClientResolver.load drifted from DetailResolver.load - it never got the null guard.  The server answers
//{"data":{"serverConnection":null}} for a slug it does not have, and the null then TypeError'd on obj["id"] in the
//subQueries loop, so a missing row and a malformed query arrived at the catch as the same throw.
describe( 'ClientResolver.load', ()=>{
	const routing = new DetailRoute( 'plc1', undefined, [], <any>{path:'.', title:'gw'} );
	const snackbar = { exception: vi.fn(), error: vi.fn() } as unknown as SnackbarService;
	const opcStore = { getConnection: async ()=>({}) } as unknown as OpcStore;

	const gateway = ( querySingle:()=>Promise<any> )=>(<unknown>{
		schemaWithEnums: async ()=>({collectionName:'serverConnections', type:'ServerConnection'}),
		slugQuery: ()=>'serverConnection(...)',
		subQueries: ()=>['opcSessions{count}'],
		querySingle,
		query: async ()=>({opcSessions: []})
	}) as Gateway;

	it( 'throws SlugNotFoundError for a row the server does not have', async ()=>{
		const ql = gateway( async ()=>null );
		await expect( ClientResolver.load(ql, opcStore, 'plc1', routing, snackbar) ).rejects.toBeInstanceOf( SlugNotFoundError );
	} );

	it( 'lets a real query failure through as itself', async ()=>{
		const ql = gateway( async ()=>{ throw new Error("(500)malformed query"); } );
		//the point of the guard: this must NOT be reported as "slug not found"
		await expect( ClientResolver.load(ql, opcStore, 'plc1', routing, snackbar) ).rejects.not.toBeInstanceOf( SlugNotFoundError );
	} );

	it( 'still resolves a row that exists', async ()=>{
		const ql = gateway( async ()=>({id: 4, slug: 'plc1'}) );
		const data = await ClientResolver.load( ql, opcStore, 'plc1', routing, snackbar );
		expect( data.row.id ).toBe( 4 );
		expect( data.row.opcSessions ).toEqual( [] );
	} );

	it( 'skips the query for $new', async ()=>{
		const querySingle = vi.fn();
		const data = await ClientResolver.load( gateway(querySingle as any), opcStore, '$new', routing, snackbar );
		expect( querySingle ).not.toHaveBeenCalled();
		expect( data.row ).toEqual( {} );
	} );
} );

//review3 C1: the body is DetailResolver.load now.  These pin what delegating must NOT have changed - the gateway query
//still carries no `variables` param, and the opcStore fetch is still Client-specific and still non-fatal.
describe( 'ClientResolver.load delegation', ()=>{
	const routing = new DetailRoute( 'plc1', undefined, [], <any>{path:'.', title:'gw'} );
	const snackbar = { exception: vi.fn(), error: vi.fn() } as unknown as SnackbarService;

	const spyGateway = ( calls:any[] )=>(<unknown>{
		schemaWithEnums: async ()=>({collectionName:'serverConnections', type:'ServerConnection'}),
		slugQuery: ( _s:any, slug:string )=>`serverConnection(slug:"${slug}")`,
		subQueries: ()=>[],
		querySingle: async ( ql:string, vars:any )=>{ calls.push({ql, vars}); return {id: 4, slug:'plc1'}; },
		query: async ()=>({})
	}) as Gateway;

	it( 'sends no variables, as the gateway query always has', async ()=>{
		const calls:any[] = [];
		await ClientResolver.load( spyGateway(calls), {getConnection: async ()=>({})} as unknown as OpcStore, 'plc1', routing, snackbar );
		expect( calls ).toHaveLength( 1 );
		expect( calls[0].vars ).toBeNull();//NOT DetailResolver's default {} - ql() appends `&variables=` for anything truthy
	} );

	it( 'resolves the row even when the server connection cannot be reached', async ()=>{
		const failing = { getConnection: async ()=>{ throw new Error("no route to host"); } } as unknown as OpcStore;
		const exception = vi.fn();
		const data = await ClientResolver.load( spyGateway([]), failing, 'plc1', routing, {exception, error: vi.fn()} as unknown as SnackbarService );
		expect( data.row.id ).toBe( 4 );//a page that cannot reach the OPC server still has its connection row
		expect( data.row.server ).toBeUndefined();
		expect( data.row.serverError ).toBe( "no route to host" );//the Connection tab's not-connected state shows it after the toast has gone
		expect( exception ).toHaveBeenCalledWith( "Could not connect to server.", expect.any(Error) );
	} );

	it( 'never asks the opcStore for $new', async ()=>{
		const getConnection = vi.fn();
		await ClientResolver.load( spyGateway([]), {getConnection} as unknown as OpcStore, '$new', routing, snackbar );
		expect( getConnection ).not.toHaveBeenCalled();
	} );
} );
