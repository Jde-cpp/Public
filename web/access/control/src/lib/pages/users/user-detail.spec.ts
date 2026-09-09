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
import { Operator, QLListResolver, SnackbarService, TableSchema, View } from 'jde-framework';
import { AccessService } from '../../services/access-service';
import { resourceTableSettings, UserDetail, userTableSettings } from './user-detail';

//angular-review3 L2: only group-detail clamped the stored tab index on $new.  The other detail pages restored an index
//that names a tab the @if drops for a new record, and mat-tab-group hard-loops on a selectedIndex it cannot resolve.
const create = ( row:any, mutations?:string[] )=>{
	TestBed.configureTestingModule({ providers: [
		{ provide: ActivatedRoute, useValue: {data: of({pageData: {row, routing: {}, schema: {enums: new Map()}}})} },
		{ provide: Router, useValue: {navigate: ()=>{}} },
		{ provide: ComponentPageTitle, useValue: {} },
		{ provide: SnackbarService, useValue: {exception: ()=>{}} },
		{ provide: AccessService, useValue: {mutate: async ( ql:string )=>{ mutations?.push( ql ); }} }
	]});
	const page = TestBed.createComponent( UserDetail ).componentInstance;
	page.ngOnInit();//the route.data subscription lives in ngOnInit since C2 - createComponent alone does not call it
	return page;
};

describe( 'UserDetail tab index', ()=>{
	beforeEach( ()=>localStorage.setItem('userDetail', '3') );//Permissions - the last tab an existing user has
	afterEach( ()=>localStorage.removeItem('userDetail') );

	it( 'clamps to Properties for a new user', ()=>{
		expect( create({}).tabIndex() ).toBe( 0 );
	} );

	it( 'keeps the stored index for an existing user', ()=>{
		expect( create({id: 7, name: "bob"}).tabIndex() ).toBe( 3 );
	} );
} );

//The soft delete client-detail has had:  the generic delete<Type>/restore<Type> the ql schema advertises for every table.
describe( 'UserDetail delete', ()=>{
	it( 'is offered for a saved user', ()=>{
		expect( create({id: 7, name: "bob"}).isNew ).toBe( false );
	} );

	it( 'is disabled for a new user', ()=>{//TestBed takes one configure per test - the two rows cannot share an `it`
		expect( create({}).isNew ).toBe( true );
	} );

	it( 'deletes by id', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 7, name: "bob"}, sent );
		expect( page.isDeleted ).toBe( false );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["deleteUser(id:7)"] );
	} );

	it( 'restores a row the show-deleted view returned', async ()=>{
		const sent:string[] = [];
		const page = create( {id: 7, name: "bob", deleted: "2026-09-09T00:00:00Z"}, sent );
		expect( page.isDeleted ).toBe( true );
		await page.onDeleteClick();
		expect( sent ).toEqual( ["restoreUser(id:7)"] );
	} );
} );

//the server authenticates a logon by loginName+provider (AuthenticateAwait), so both are fixed once the row exists - and a
//key user's provider doubly so, since changing it swaps key-properties for the generic form mid-edit.  One list feeds both forms.
describe( 'UserDetail readonly fields', ()=>{
	it( 'fixes provider and login name on a saved user', ()=>{
		expect( create({id: 7, name: "bob"}).readonlyFields ).toEqual( ["provider", "loginName"] );
	} );

	it( 'leaves both settable on a new user', ()=>{
		expect( create({}).readonlyFields ).toEqual( [] );
	} );
} );

//review3 L6: 'resources' is the one collection under access's ':collectionDisplay' with no sibling ':target' detail
//route, so a row click there dead-ended in a NavigationError.  QLList honours canNavigate; this is the wiring.
describe( 'resourceTableSettings', ()=>{
	it( 'does not offer a row click-through', ()=>{
		expect( resourceTableSettings.canNavigate ).toBe( false );
	} );
} );

//The users list's three system views.  'Users' and 'Certs' began life as one user's saved views; they are declared on
//the route now so every user gets them, and the default view is labelled 'All' instead of the framework's 'default'.
describe( 'userTableSettings views', ()=>{
	const scalar = ( name:string, type:string="String" )=>({ name, type: { kind: "SCALAR", name: type } });
	const schema = new TableSchema( { name: "User", fields: [
		{ name: "id", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "ID" } } },
		{ name: "name", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "target", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "provider", type: { kind: "OBJECT", name: "Provider" } },
		scalar("email"), scalar("loginName"), scalar("modulus"), scalar("issuer"), scalar("distinguished"), scalar("subjectAlt"),
		scalar("expiration", "DateTime"), scalar("deleted", "DateTime"), scalar("description"), scalar("isGroup", "Boolean")
	] } );
	const displayed = ( v:View )=>v.fields.filter( f=>f.displayed ).map( f=>f.name );
	const views = QLListResolver.systemViews( schema, userTableSettings );
	const byName = ( name:string )=>views.find( v=>v.name==name )!;

	it( 'are All, Users and Certs - system views, in that order', ()=>{
		expect( views.map(v=>v.name) ).toEqual( ["All", "Users", "Certs"] );
		expect( views.every(v=>v.isSystem) ).toBe( true );
	} );

	it( 'Users lists the non-certificate identities with their login columns', ()=>{
		expect( displayed(byName("Users")) ).toEqual( ["name", "provider", "email", "loginName", "description"] );
		const q = byName( "Users" ).query( false, 0 );
		expect( q.text ).toContain( "issuer:$issuer" );
		expect( q.vars["issuer"] ).toEqual( [null] );
	} );

	it( 'Certs lists the key identities with their certificate columns', ()=>{
		expect( displayed(byName("Certs")) ).toEqual( ["target", "modulus", "issuer", "distinguished", "subjectAlt", "expiration", "description"] );
		expect( byName("Certs").fieldFilters.map(f=>[f.field.name, f.filter.operator, f.filter.value]) ).toEqual( [["provider", Operator.In, ["Key"]]] );
		expect( byName("Certs").sort ).toEqual( [{active: "target", direction: "asc"}] );//name is not a displayed column there
	} );
} );
