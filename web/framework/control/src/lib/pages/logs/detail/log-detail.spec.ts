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
import { vi } from 'vitest';
import { ProfileStore } from 'jde-spa';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { IGraphQL } from '../../../services/graphql';
import { LogSettings } from '../log-settings';
import { LogDetail } from './log-detail';

//review3 C3: the vestigial subscription machinery is gone - subscribe()/unsubscribe(), currentSubscription, the buffer,
//onStrings, applications, connected, paused, start.  `subscription` was never assigned, so unsubscribe() was a permanent
//no-op and every one of its callers was reachable only from commented-out markup.  These pin what SURVIVED.
describe( 'LogDetail after the subscription cull', ()=>{
	let ql:any;
	let save:any;
	let exception:any;

	const entries = ( count:number )=>({ entries: Array.from({length:count}, (_,i)=>({templateId:1, argIds:[], level:3, tags:[], line:i, time:new Date().toISOString(), userId:0, fileId:0, functionId:0})), strings: [] });

	const create = ()=>{
		save = vi.fn().mockResolvedValue( undefined );
		exception = vi.fn();
		ql = { ql: vi.fn().mockResolvedValue({logs: entries(2)}) };
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			{ provide: SnackbarService, useValue: {exception, warn: vi.fn()} },
			{ provide: ProfileStore, useValue: {
				load: async ( _key:string, dflt:any )=>dflt,
				loadClassArray: async ()=>[],
				save
			} }
		]});
		const fixture = TestBed.createComponent( LogDetail );
		fixture.componentRef.setInput( 'service', <IGraphQL>ql );
		return fixture.componentInstance;
	};

	it( 'loads its first page', async ()=>{
		const page = create();
		await page.ngOnInit();
		expect( ql.ql ).toHaveBeenCalledTimes( 1 );
		expect( page.isLoading() ).toBe( false );
		expect( page.data.allEntries.length ).toBe( 2 );
	} );

	it( 'reports a failed load instead of leaving the spinner up forever', async ()=>{
		const page = create();
		ql.ql.mockRejectedValue( new Error("gateway down") );
		await page.ngOnInit();
		expect( exception ).toHaveBeenCalledWith( "Could not load log entries.", expect.any(Error) );
	} );

	it( 're-reads from the top on refresh', async ()=>{
		const page = create();
		await page.ngOnInit();
		await page.refresh();
		expect( ql.ql ).toHaveBeenCalledTimes( 2 );
	} );

	//ngOnDestroy used to call unsubscribe() first; only the profile save was ever doing anything.
	it( 'still saves the log profile on the way out', ()=>{
		const page = create();
		page.profile = new LogSettings( new LogSettings() );
		page.ngOnDestroy();
		expect( save ).toHaveBeenCalledWith( "logs", page.profile );
	} );

	it( 'no longer offers the subscription surface', ()=>{
		const page:any = create();
		for( const gone of ['subscribe', 'unsubscribe', 'onStrings', 'onChangeApplication', 'onLevelChange', 'startChange'] )
			expect( gone in page ).toBe( false );
	} );
} );
