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
import { RoleDetail } from './role-detail';

//group-detail.spec's harness, verbatim - every access detail page is a DetailPage subclass loading through `pageData`.
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
	const page = TestBed.createComponent( RoleDetail ).componentInstance;
	page.ngOnInit();//the route.data subscription lives in ngOnInit - createComponent alone does not call it
	return page;
};

//Roles are their own table, not the identities view groups and users share, so unlike `deleteGroup` these three mutations
//are advertised by the schema outright: delete/restore/purgeRole are all in the hub's mutation list.
describe( 'RoleDetail delete', ()=>{
	it( 'is offered for a saved role', ()=>{
		expect( create({id: 3, name: "Admins"}).isNew ).toBe( false );
	} );

	it( 'is disabled for a new role', ()=>{//TestBed takes one configure per test - the two rows cannot share an `it`
		expect( create({}).isNew ).toBe( true );
	} );

	it( 'deletes by id', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 3, name: "Admins"}, sent );
		expect( page.isDeleted ).toBe( false );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["deleteRole(id:3)"] );
	} );

	it( 'restores a row the show-deleted view returned', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 3, name: "Admins", deleted: "2026-09-09T00:00:00Z"}, sent );
		expect( page.isDeleted ).toBe( true );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["restoreRole(id:3)"] );
	} );
} );

describe( 'RoleDetail purge', ()=>{
	afterEach( ()=>{ confirmed = true; } );

	it( 'purges by id once confirmed', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 3, name: "Admins", deleted: "2026-09-09T00:00:00Z"}, sent );
		await page.onPurgeClick();
		expect( sent ).toEqual( ["purgeRole(id:3)"] );
	} );

	it( 'sends nothing when the confirmation is declined', async ()=>{
		confirmed = false;
		const sent:string[] = [];
		const page = create( {id: 3, name: "Admins", deleted: "2026-09-09T00:00:00Z"}, sent );
		await page.onPurgeClick();
		expect( sent ).toEqual( [] );
	} );
} );
