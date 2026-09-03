import { TestBed } from '@angular/core/testing';
import { OpcObject, UaNode } from '../model/node';
import { OpcStore } from './opc-store';

const gateway = "gw", cnnctn = "local";
//browse carries the {ns,name} pair findNodeId matches on; a distinct numeric id keeps every node's store entry its own.
const node = ( id:number, name:string ):UaNode => new OpcObject( {ns: 2, i: id, name, browse: {ns: 2, name}} );

describe( 'OpcStore.findNodeId', ()=>{
	let store:OpcStore;
	//The partial cache insertNode manufactures with its `store.children = []` single-child reset:  root has 'a' and 'c',
	//and 'a' has a child 'c' of its own - but 'a' has no 'x'.  OPC DI trees repeat names like this at every device level.
	const a = node( 1, "a" ), rootC = node( 2, "c" ), aC = node( 3, "c" );
	beforeEach( ()=>{
		TestBed.configureTestingModule({});
		store = TestBed.inject( OpcStore );
		store.setNodes( gateway, cnnctn, OpcObject.rootNode, [a, rootC] );
		store.setNodes( gateway, cnnctn, a, [aC] );
	} );

	it( 'resolves a path that is fully cached', ()=>{
		expect( store.findNodeId(gateway, cnnctn, "2~a") ).toBe( a );
		expect( store.findNodeId(gateway, cnnctn, "2~a/2~c") ).toBe( aC );
		expect( store.findNodeId(gateway, cnnctn, "2~c") ).toBe( rootC );
	} );

	//angular-review3 #8: the walk was a forEach, which cannot break.  'x' missed, `storeNode` stayed on 'a', and the next
	//segment matched a's OWN 'c' - so the url a/x/c resolved to a node that is not on that path at all.  NodeResolver takes
	//any non-null answer as authoritative and skips the server query.
	it( 'fails the whole walk when a MIDDLE segment misses', ()=>{
		expect( store.findNodeId(gateway, cnnctn, "2~a/2~x/2~c") ).toBeUndefined();
	} );

	it( 'fails when the first segment misses', ()=>{
		expect( store.findNodeId(gateway, cnnctn, "2~x/2~c") ).toBeUndefined();
	} );

	it( 'fails when the last segment misses', ()=>{
		expect( store.findNodeId(gateway, cnnctn, "2~a/2~x") ).toBeUndefined();
	} );

	it( 'answers undefined for a connection it has never seen', ()=>{
		expect( store.findNodeId(gateway, "other", "2~a") ).toBeUndefined();
	} );
} );
