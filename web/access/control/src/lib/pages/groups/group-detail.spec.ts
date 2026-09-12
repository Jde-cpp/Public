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
import { MatDialog } from '@angular/material/dialog';
import { AccessService } from '../../services/access-service';
import { GroupDetail } from './group-detail';

//user-detail.spec's harness, verbatim - both pages are DetailPage subclasses loading through the route's `pageData`.
let confirmed = true;//what the stubbed confirmation dialog answers
const create = ( row:any, mutations?:string[] )=>{
	TestBed.configureTestingModule({ providers: [
		{ provide: ActivatedRoute, useValue: {data: of({pageData: {row, routing: {}, schema: {enums: new Map()}}})} },
		{ provide: Router, useValue: {navigate: ()=>{}} },
		{ provide: ComponentPageTitle, useValue: {} },
		{ provide: SnackbarService, useValue: {exception: ()=>{}} },
		{ provide: AccessService, useValue: {mutate: async ( ql:string )=>{ mutations?.push( ql ); }} },
		{ provide: MatDialog, useValue: {open: ()=>({afterClosed: ()=>({subscribe: ( f:( y:boolean )=>void )=>f( confirmed )})})} }
	]});
	const page = TestBed.createComponent( GroupDetail ).componentInstance;
	page.ngOnInit();//the route.data subscription lives in ngOnInit - createComponent alone does not call it
	return page;
};

//The group page had no Delete at all until the trio moved up into DetailPage:  a group is an identity row like a user, and
//the generic delete<Type>/restore<Type> mutations cover it - `deleteGroup(id:N)` stamps identities.deleted, verified live.
describe( 'GroupDetail delete', ()=>{
	it( 'is offered for a saved group', ()=>{
		expect( create({id: 6, name: "OPC Gateways"}).isNew ).toBe( false );
	} );

	it( 'is disabled for a new group', ()=>{//TestBed takes one configure per test - the two rows cannot share an `it`
		expect( create({}).isNew ).toBe( true );
	} );

	it( 'deletes by id', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 6, name: "OPC Gateways"}, sent );
		expect( page.isDeleted ).toBe( false );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["deleteGroup(id:6)"] );
	} );

	it( 'restores a row the show-deleted view returned', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 6, name: "OPC Gateways", deleted: "2026-09-09T00:00:00Z"}, sent );
		expect( page.isDeleted ).toBe( true );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["restoreGroup(id:6)"] );
	} );
} );

//Purge is the hard delete, so the template offers it only on an already-deleted row and DetailPage puts a confirmation in
//front of it.  Verified against the live hub: `purgeGroup(id:N)` removes the row outright - `groups(id:N)` comes back empty.
describe( 'GroupDetail purge', ()=>{
	afterEach( ()=>{ confirmed = true; } );

	it( 'purges by id once confirmed', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 6, name: "OPC Gateways", deleted: "2026-09-09T00:00:00Z"}, sent );
		await page.onPurgeClick();
		expect( sent ).toEqual( ["purgeGroup(id:6)"] );
	} );

	it( 'sends nothing when the confirmation is declined', async ()=>{
		confirmed = false;
		const sent:string[] = [];
		const page = create( {id: 6, name: "OPC Gateways", deleted: "2026-09-09T00:00:00Z"}, sent );
		await page.onPurgeClick();
		expect( sent ).toEqual( [] );
	} );
} );
