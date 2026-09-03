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
import { AccessService } from '../../services/access-service';
import { resourceTableSettings, UserDetail } from './user-detail';

//angular-review3 L2: only group-detail clamped the stored tab index on $new.  The other detail pages restored an index
//that names a tab the @if drops for a new record, and mat-tab-group hard-loops on a selectedIndex it cannot resolve.
const create = ( row:any )=>{
	TestBed.configureTestingModule({ providers: [
		{ provide: ActivatedRoute, useValue: {data: of({pageData: {row, routing: {}, schema: {enums: new Map()}}})} },
		{ provide: Router, useValue: {navigate: ()=>{}} },
		{ provide: ComponentPageTitle, useValue: {} },
		{ provide: SnackbarService, useValue: {exception: ()=>{}} },
		{ provide: AccessService, useValue: {mutate: async ()=>{}} }
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

//review3 L6: 'resources' is the one collection under access's ':collectionDisplay' with no sibling ':target' detail
//route, so a row click there dead-ended in a NavigationError.  QLList honours canNavigate; this is the wiring.
describe( 'resourceTableSettings', ()=>{
	it( 'does not offer a row click-through', ()=>{
		expect( resourceTableSettings.canNavigate ).toBe( false );
	} );
} );
