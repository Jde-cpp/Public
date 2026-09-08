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
import { ActivatedRoute, Router } from '@angular/router';
import { of } from 'rxjs';
import { ComponentPageTitle } from 'jde-spa';
import { SnackbarService } from 'jde-framework';
import { GatewayService } from '../../../services/gateway-service';
import { ClientDetail } from './client-detail';

//angular-review3 L2: only group-detail clamped the stored tab index.  Here the Connection tab is gated on the row being
//saved, so a stored index of 1 named a tab that does not exist for a new connection - mat-tab-group hard-loops on an index
//it cannot resolve.  An existing connection whose server is unreachable keeps the tab (in its not-connected state).
const create = ( row:any )=>{
	TestBed.configureTestingModule({ providers: [
		{ provide: ActivatedRoute, useValue: {data: of({pageData: {row, routing: {}, schema: {enums: new Map()}}})} },
		{ provide: Router, useValue: {navigate: ()=>{}, url: "/gateways/g/clients/$new"} },
		{ provide: ComponentPageTitle, useValue: {} },
		{ provide: SnackbarService, useValue: {exception: ()=>{}} },
		{ provide: GatewayService, useValue: {gateway: async ()=>({})} }
	]});
	const page = TestBed.createComponent( ClientDetail ).componentInstance;
	page.ngOnInit();//the route.data subscription lives in ngOnInit since C2 - createComponent alone does not call it
	return page;
};

describe( 'ClientDetail tab index', ()=>{
	beforeEach( ()=>localStorage.setItem('client-detail', '1') );//Connection
	afterEach( ()=>localStorage.removeItem('client-detail') );

	it( 'clamps to Properties for a new connection', ()=>{
		expect( create({}).tabIndex() ).toBe( 0 );
	} );

	it( 'keeps the stored index when the Connection tab exists', ()=>{
		expect( create({id: 7, name: "plc", server: {id: 2}}).tabIndex() ).toBe( 1 );
	} );

	it( 'keeps the stored index for a saved connection whose server is unreachable', ()=>{
		expect( create({id: 7, name: "plc", serverError: "no route to host"}).tabIndex() ).toBe( 1 );
	} );
} );
