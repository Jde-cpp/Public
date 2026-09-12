import { TestBed } from '@angular/core/testing';
import { NodeId } from '../model/node-id';
import { GATEWAY_SERVICE } from './gateway-service';
import { OPC_STORE } from './opc-store';
import { OpcNodeLinkResolver } from './node-link-resolver';

//Two gateways, three connections; only 'local' on 'gw2' is the server whose applicationName brackets "debug" - the
//accessResource node-access grants under, and the <x> of an "opc.<x>" resource schema.  The gateway's `node( id ){ path }`
//is stubbed:  the pump exists, node 9 answers null (outside the Objects tree), and 'down' cannot even be described.
describe( 'OpcNodeLinkResolver', ()=>{
	const queries:string[] = [];
	const gateway = ( target:string, connections:string[] )=>({
		target,
		queryArray: async ()=>connections.map( c=>({target: c}) ),
		querySingle: async ( ql:string, vars:any )=>{ queries.push( `${target}/${vars.opc}/${new NodeId(vars.id).uaString()}` ); return vars.id.i==5005 ? { name: "Pump 1", path: "pumps/pump1" } : { name: "Type", path: null }; }
	});
	const gw1 = gateway( "gw1", ["down", "other"] ), gw2 = gateway( "gw2", ["local"] );
	const described:string[] = [];
	const store = { getConnection: async ( g:any, cnnctn:string )=>{
		described.push( `${g.target}/${cnnctn}` );
		if( cnnctn=="down" ) throw new Error( "unreachable" );
		return { accessResource: cnnctn=="local" ? "debug" : "other" };
	} };
	let resolver:OpcNodeLinkResolver;
	beforeEach( ()=>{
		queries.length = 0; described.length = 0;
		TestBed.configureTestingModule({ providers: [
			{ provide: GATEWAY_SERVICE, useValue: { gateways: async ()=>[gw1, gw2] } },
			{ provide: OPC_STORE, useValue: store }
		]});
		resolver = TestBed.inject( OpcNodeLinkResolver );
	} );

	it( 'places a node on the connection whose server carries the schema, and routes by the browse path', async ()=>{
		expect( await resolver.resolve("opc.debug", "ns=5;i=5005") ).toEqual( { route: ['/gateways', 'gw2', 'local', 'pumps', 'pump1'], name: "Pump 1", path: "pumps/pump1" } );
		expect( described ).toEqual( ["gw1/down", "gw1/other", "gw2/local"] );//an undescribable server is skipped, not fatal
		expect( queries ).toEqual( ["gw2/local/ns=5;i=5005"] );
	} );

	it( 'remembers the placement, not a miss', async ()=>{
		await resolver.resolve( "opc.debug", "ns=5;i=5005" );
		await resolver.resolve( "opc.debug", "ns=5;i=9" );
		expect( described ).toHaveLength( 3 );//the second call reused gw2/local
		expect( await resolver.resolve("opc.nowhere", "ns=5;i=5005") ).toBeUndefined();
		await resolver.resolve( "opc.nowhere", "ns=5;i=5005" );
		expect( described ).toHaveLength( 9 );//a miss is looked up again - the server may be up next time
	} );

	it( 'gives no link for a node the gateway cannot place, or a schema that is not a server', async ()=>{
		expect( await resolver.resolve("opc.debug", "ns=5;i=9") ).toBeUndefined();
		expect( await resolver.resolve("access", "x") ).toBeUndefined();
		expect( queries ).toEqual( ["gw2/local/ns=5;i=9"] );
	} );
} );

describe( 'NodeId.fromUaString', ()=>{
	it( 'round-trips uaString for every identifier kind', ()=>{
		for( const s of ["ns=5;i=5005", "i=85", "ns=2;s=pump 1", "ns=3;g=12345678-1234-1234-1234-123456789abc"] )
			expect( NodeId.fromUaString(s).uaString() ).toBe( s );
	} );
	it( 'refuses anything else', ()=>{
		expect( ()=>NodeId.fromUaString("pump1") ).toThrow( /not a NodeId/ );
	} );
} );
