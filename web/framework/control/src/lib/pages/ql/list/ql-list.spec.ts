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
import { NEVER } from 'rxjs';
import { vi } from 'vitest';
import { ComponentPageTitle } from 'jde-spa';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { ListRoute, QLListData, QLListResolver, TableSettings } from '../../../services/ql-list-resolver';
import { TableSchema } from '../../../model/ql/schema/table-schema';
import { View } from '../../../model/ql/view';
import { PageProfile } from '../../graphql/model/page-settings';
import { QLList } from './ql-list';

//review3 L6: /access/resources has no 'resources/:target' route, so every row click there dead-ended in a
//NavigationError - and the try/catch around router.navigate could never report it, navigate being async.
describe( 'QLList.onRowActivate', ()=>{
	let navigate:any;
	let error:any;
	let exception:any;

	const create = ( tableSettings:TableSettings )=>{
		navigate = vi.fn().mockResolvedValue( true );
		error = vi.fn();
		exception = vi.fn();
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			{ provide: ActivatedRoute, useValue: {data: NEVER, routeConfig: {}} },
			{ provide: Router, useValue: {navigate} },
			{ provide: ComponentPageTitle, useValue: {} },
			{ provide: 'IGraphQL', useValue: {} },
			{ provide: SnackbarService, useValue: {error, exception} }
		]});
		const page = TestBed.createComponent( QLList ).componentInstance;
		//routing is what onRowActivate reads; schema/profile only keep ngOnDestroy's showDeleted save alive (ngOnInit never runs)
		page.resolvedData.set( {
			routing: new ListRoute( {path:'resources', data:{tableSettings} as any} ),
			schema: {collectionName: 'resources'},
			profile: {showDeleted: false}
		} as QLListData );
		return page;
	};

	it( 'navigates for a collection that has a detail route', ()=>{
		create( {} ).onRowActivate( {target: 'someone'} );
		expect( navigate ).toHaveBeenCalledWith( ['someone'], expect.anything() );
	} );

	it( 'does not navigate where canNavigate is off', ()=>{
		create( {canNavigate: false} ).onRowActivate( {target: 'nodes'} );
		expect( navigate ).not.toHaveBeenCalled();
	} );

	//the try/catch it replaced could not see either outcome: navigate settles after the block has returned.
	it( 'reports a navigation the router refused', async ()=>{
		const page = create( {} );
		navigate.mockResolvedValue( false );
		page.onRowActivate( {target: 'someone'} );
		await Promise.resolve();
		expect( error ).toHaveBeenCalledWith( "Could not navigate to 'someone'." );
	} );

	it( 'reports a navigation that threw', async ()=>{
		const page = create( {} );
		navigate.mockRejectedValue( new Error("Cannot match any routes") );
		page.onRowActivate( {target: 'someone'} );
		await Promise.resolve();
		await Promise.resolve();
		expect( exception ).toHaveBeenCalledWith( "Could not navigate to properties", expect.any(Error) );
	} );
} );

//review3 L7: onViewShow/onChangeView/onToggleShowDeleted fired refresh() without awaiting it, and onViewSave/onViewDelete
//awaited reload() with no catch - so a failed re-query left the table empty or stale and said nothing, the rejection
//going to the console as unhandled.  Every one now reports through #refresh/#reload.
describe( 'QLList re-query failures reach the user', ()=>{
	let exception:any;

	const schema = new TableSchema( { name: "User", fields: [
		{ name: "id", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "ID" } } },
		{ name: "name", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "target", type: { kind: "SCALAR", name: "String" } },
		{ name: "deleted", type: { kind: "SCALAR", name: "DateTime" } }
	] } );

	const create = ()=>{
		exception = vi.fn();
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			{ provide: ActivatedRoute, useValue: {data: NEVER, routeConfig: {}} },
			{ provide: Router, useValue: {navigate: vi.fn().mockResolvedValue(true)} },
			{ provide: ComponentPageTitle, useValue: {} },
			{ provide: 'IGraphQL', useValue: {} },
			{ provide: SnackbarService, useValue: {error: vi.fn(), exception} }
		]});
		const page = TestBed.createComponent( QLList ).componentInstance;
		const view = new View( {name: "All", columns: ["name"], sort: "name"}, schema );
		const profile = new PageProfile();
		profile.showDeleted = false;
		profile.views = [view];
		page.resolvedData.set( {routing: new ListRoute({path:'users'}), schema, profile} as unknown as QLListData );
		page.view.set( view );
		//the shared failure: QLListResolver.load is where every re-query path ends up
		vi.spyOn( QLListResolver, 'load' ).mockRejectedValue( new Error("gateway down") );
		return page;
	};

	afterEach( ()=>vi.restoreAllMocks() );

	it( 'reports a failed view change', async ()=>{
		await create().onChangeView( 0 );
		expect( exception ).toHaveBeenCalledWith( "Could not refresh data.", expect.any(Error) );
	} );

	it( 'reports a failed show-deleted toggle', async ()=>{
		await create().onToggleShowDeleted();
		expect( exception ).toHaveBeenCalledWith( "Could not refresh data.", expect.any(Error) );
	} );

	it( 'reports a failed view show', async ()=>{
		const page = create();
		await page.onViewShow( page.view() );
		expect( exception ).toHaveBeenCalledWith( "Could not refresh data.", expect.any(Error) );
	} );

	it( 'reports a failed reload behind Save', async ()=>{
		const page = create();
		await page.onViewSave( page.view() );
		expect( exception ).toHaveBeenCalledWith( "Could not refresh data.", expect.any(Error) );
	} );

	it( 'clears isRefreshing even when the re-query fails', async ()=>{
		const page = create();
		await page.onChangeView( 0 );
		expect( page.isRefreshing() ).toBe( false );
	} );
} );
