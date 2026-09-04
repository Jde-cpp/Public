if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { HttpClient } from '@angular/common/http';
import { AuthStore, ETransport } from 'jde-framework';
import { NodeId } from '../model/node-id';
import { OpcError } from '../model/opc-error';
import { scBadUnexpectedError } from '../model/types';
import { Gateway, SubscriptionResult } from './gateway-service';
import { OpcStore } from './opc-store';

const opcId = "local";
const A = new NodeId( {ns:2, i:1} ), B = new NodeId( {ns:2, i:2} );

class TestGateway extends Gateway{
	constructor(){
		super( {host:'localhost', port:1968, instanceName:'gw'} as any, ETransport.Unsecure, {} as HttpClient,
			{user: ()=>undefined, logout: ()=>{}} as unknown as AuthStore, new OpcStore() );
	}
	override async ql<Y>():Promise<Y>{ return {serverConnections: []} as Y; }//the constructor's connection query - no http here
	//the subscribe reply the server would send, or a rejection standing in for a send that never got there.
	statusCodes:( number|undefined )[] = [];
	rejectWith?:any;
	sent:any[] = [];
	override sendPromise<T>( m:any, log:string ):Promise<T>{
		this.sent.push( m );
		if( !m.subscribe )
			return Promise.resolve( undefined as T );
		if( this.rejectWith )
			return Promise.reject( this.rejectWith );
		return Promise.resolve( this.statusCodes.map( statusCode=>({statusCode}) ) as T );
	}
	get unsubscribed():NodeId[]{ return this.sent.filter( m=>m.unsubscribe ).flatMap( m=>m.unsubscribe.nodes ); }
}
//`unsubscribe` only sends for a node this owner is actually registered on, so a send is proof the registration survived.
const stillRegistered = async ( gateway:TestGateway, node:NodeId, owner:string )=>{
	const before = gateway.unsubscribed.length;
	await gateway.unsubscribe( opcId, [node], owner );
	return gateway.unsubscribed.length>before;
};

describe( 'Gateway subscribe failures', ()=>{
	let gateway:TestGateway;
	beforeEach( ()=>{ gateway = new TestGateway(); } );

	//angular-review3 #9: the per-node failure path deleted the whole per-node-key entry, so a key a SECOND owner also held
	//lost that owner's registration too - the server kept pushing and nodeValues' forEach silently no-oped.
	it( 'a per-node failure drops only the failing owner from a shared node', async ()=>{
		gateway.statusCodes = [undefined];
		gateway.subscribe( opcId, [A], "owner1" ).subscribe( {next:()=>{}, error:()=>{}} );
		await Promise.resolve();
		gateway.statusCodes = [0x80340000];//BadNodeIdUnknown for owner2's request
		gateway.subscribe( opcId, [A], "owner2" ).subscribe( {next:()=>{}, error:()=>{}} );
		await Promise.resolve();
		expect( await stillRegistered(gateway, A, "owner1") ).toBe( true );
		expect( await stillRegistered(gateway, A, "owner2") ).toBe( false );
	} );

	//...and the catch called clearOwner, unsubscribing the owner's already-live nodes and erroring the shared Subject.
	it( 'a send error leaves the owner\'s other live nodes alone', async ()=>{
		const results:SubscriptionResult[] = [];
		let errored:any;
		gateway.statusCodes = [undefined];
		gateway.subscribe( opcId, [A], "owner1" ).subscribe( {next: r=>results.push(r), error: e=>errored=e} );
		await Promise.resolve();
		gateway.rejectWith = { error: {requestId: 1, message: "Connection lost."} };
		gateway.addToSubscription( opcId, [B], "owner1" );
		await Promise.resolve(); await Promise.resolve();

		expect( errored ).toBeUndefined();//erroring the Subject ended subscriptions that had nothing to do with the request
		expect( gateway.unsubscribed ).toHaveLength( 0 );//nothing reached the server, so there is nothing to unsubscribe
		expect( await stillRegistered(gateway, A, "owner1") ).toBe( true );
	} );

	it( 'reports the failed node on the stream as a bad reading', async ()=>{
		const results:SubscriptionResult[] = [];
		gateway.statusCodes = [undefined];
		gateway.subscribe( opcId, [A], "owner1" ).subscribe( {next: r=>results.push(r), error: ()=>{}} );
		await Promise.resolve();
		gateway.rejectWith = { error: {requestId: 1, message: "Connection lost."} };
		gateway.addToSubscription( opcId, [B], "owner1" );
		await Promise.resolve(); await Promise.resolve();

		expect( results ).toHaveLength( 1 );
		expect( results[0].node.equals(B) ).toBe( true );
		expect( results[0].value ).toBeInstanceOf( OpcError );
		expect( results[0].sc ).toBe( scBadUnexpectedError );//no per-node code to report - the request never got there
	} );

	it( 'carries the server\'s own status code through when it gave one', async ()=>{
		const results:SubscriptionResult[] = [];
		gateway.statusCodes = [0x80340000];
		gateway.subscribe( opcId, [A], "owner1" ).subscribe( {next: r=>results.push(r), error: ()=>{}} );
		await Promise.resolve(); await Promise.resolve();
		expect( results.map(r=>r.sc) ).toEqual( [0x80340000] );
		expect( (results[0].value as OpcError).sc ).toBe( 0x80340000 );
	} );
} );

describe( 'Gateway socket path', ()=>{
	it( 'upgrades on /opc - the path an OpcHub routes the gateway protocol by', ()=>{
		const gateway = new TestGateway();
		expect( (gateway as any).socketUrl ).toBe( 'ws://localhost:1968/opc' );
	});
});
