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
import { ActivatedRoute } from '@angular/router';
import { config, NEVER, Subject } from 'rxjs';
import { ComponentPageTitle } from 'jde-spa';
import { SnackbarService } from 'jde-framework';
import { GatewayService, SubscriptionResult } from '../../../services/gateway-service';
import { NodeId } from '../../../model/node-id';
import { Variable } from '../../../model/node';
import { NodeChildren } from './node-children';

//Variable's json arg is the wire shape; the UaNode base reads ns/i/name/browse straight off it.
const variable = ( id:number, name:string )=>new Variable( <any>{ns:2, i:id, name, browse:{ns:2, name}} );
const X = variable( 1, "x" ), Y = variable( 2, "y" );

describe( 'NodeChildren subscription values', ()=>{
	let page:NodeChildren;
	let pushes:Subject<SubscriptionResult>;
	let unsubscribed:{nodes:NodeId[]}[];
	let subscribes:number, adds:number;
	beforeEach( ()=>{
		pushes = new Subject<SubscriptionResult>();
		unsubscribed = [];
		subscribes = 0; adds = 0;
		const gateway = {
			subscribe: ()=>{ ++subscribes; return pushes.asObservable(); },
			addToSubscription: ()=>{ ++adds; },
			unsubscribe: ( _cnnctn:string, nodes:NodeId[] )=>{ unsubscribed.push({nodes}); return Promise.resolve(); }
		};
		TestBed.configureTestingModule({ providers: [
			{ provide: ActivatedRoute, useValue: {data: NEVER} },//no ngOnInit load - the state is set by hand below
			{ provide: 'GatewayService', useValue: {} },
			{ provide: SnackbarService, useValue: {exception: ()=>{}} },
			{ provide: ComponentPageTitle, useValue: {} }
		]});
		page = TestBed.createComponent( NodeChildren ).componentInstance;
		page.pageData = { gateway, route: {profileKey: 'k'}, server: {connection: {target: 'local', defaultBrowseNs: 2}}, nodes: [] } as any;
		page.profile = { subscriptions: [] } as any;
	} );
	const setNodes = ( nodes:Variable[], resubscribe:boolean )=>(<any>page).setNodes( nodes, resubscribe );

	//angular-review3 #12: `variables.find(...)!.value = …` threw a TypeError inside the observer - and again on every publish
	//tick - once a re-browse had dropped the row the value was for.  RxJS swallows an observer throw into its unhandled-error
	//channel rather than out of next(), so that is where the old behaviour shows.
	it( 'ignores a value for a node that is no longer a row', async ()=>{
		const unhandled:any[] = [];
		const previous = config.onUnhandledError;
		config.onUnhandledError = e=>unhandled.push( e );
		try{
			setNodes( [X, Y], true );
			page.onSubscriptionChange( {added: [X, Y], removed: []} as any );
			setNodes( [Y], false );//X is gone from the re-browse
			pushes.next( {opcId: 'local', node: X.nodeId, value: 7} as any );
			pushes.next( {opcId: 'local', node: Y.nodeId, value: 8} as any );
			await new Promise( r=>setTimeout(r, 0) );//onUnhandledError is reported on a later task
			expect( unhandled ).toEqual( [] );
			expect( Y.value ).toBe( 8 );//the surviving row still updates
		}
		finally{ config.onUnhandledError = previous; }
	} );

	//...and nothing unsubscribed the dropped node, so the server kept publishing it:  a new SelectionModel emits no `changed`.
	it( 'unsubscribes a persisted node the re-browse dropped, keeping the intent', ()=>{
		page.profile.subscriptions = [X.nodeId, Y.nodeId];
		setNodes( [X, Y], true );
		page.onSubscriptionChange( {added: [X, Y], removed: []} as any );
		unsubscribed.length = 0;
		setNodes( [Y], false );
		expect( unsubscribed ).toHaveLength( 1 );
		expect( unsubscribed[0].nodes.map(n=>n.key) ).toEqual( [X.key] );
		expect( page.profile.subscriptions.map(n=>n.key) ).toContain( X.key );//the intent survives, so a later browse re-subscribes
	} );

	it( 'unsubscribes nothing when every persisted node still has a row', ()=>{
		page.profile.subscriptions = [X.nodeId, Y.nodeId];
		setNodes( [X, Y], true );
		page.onSubscriptionChange( {added: [X, Y], removed: []} as any );
		unsubscribed.length = 0;
		setNodes( [X, Y], false );
		expect( unsubscribed ).toHaveLength( 0 );
	} );

	//angular-review3 #13: an errored Subscription is dead but still truthy, so the next tick took the addToSubscription
	//branch - which builds a fresh gateway Subject with NO observer.  Values were dropped while the rows showed as live.
	it( 'drops the dead subscription when the socket errors it', ()=>{
		setNodes( [X, Y], true );
		page.onSubscriptionChange( {added: [X], removed: []} as any );
		expect( page.subscription ).toBeDefined();
		pushes.error( {message: "Connection to the gateway was lost."} );//Gateway.handleConnectionError
		expect( page.subscription ).toBeUndefined();
	} );

	it( 're-subscribes rather than adding to an observer-less Subject after a drop', ()=>{
		setNodes( [X, Y], true );
		page.onSubscriptionChange( {added: [X], removed: []} as any );
		pushes.error( {message: "Connection to the gateway was lost."} );
		subscribes = 0; adds = 0;
		pushes = new Subject<SubscriptionResult>();//the gateway builds a new Subject too
		page.onSubscriptionChange( {added: [Y], removed: []} as any );//the user re-ticks a row
		expect( subscribes ).toBe( 1 );
		expect( adds ).toBe( 0 );
		pushes.next( {opcId: 'local', node: Y.nodeId, value: 9} as any );
		expect( Y.value ).toBe( 9 );//the values actually arrive now
	} );

	it( 'drops it on complete too', ()=>{
		setNodes( [X], true );
		page.onSubscriptionChange( {added: [X], removed: []} as any );
		pushes.complete();
		expect( page.subscription ).toBeUndefined();
	} );
} );
