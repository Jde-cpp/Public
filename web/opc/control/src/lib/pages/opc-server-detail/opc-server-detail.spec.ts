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
import { HttpErrorResponse } from '@angular/common/http';
import { of } from 'rxjs';
import { ComponentPageTitle, RouteStore } from 'jde-spa';
import { AppService } from 'jde-framework';
import { OpcServerService } from '../../services/opc-server-service';
import { OpcServerDetail } from './opc-server-detail';

//review3 L13: the banner was built with `${e}`, so the two shapes that actually reach production - an HttpErrorResponse
//and a {error:IError} ProtoService rejection - both rendered "[object Object]" where the failure should have been.
describe( 'OpcServerDetail error banner', ()=>{
	const create = ( thrown:any )=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			{ provide: ActivatedRoute, useValue: {params: of({instance: 'opc1'})} },
			{ provide: ComponentPageTitle, useValue: {} },
			{ provide: RouteStore, useValue: {getChildren: ()=>[]} },
			{ provide: AppService, useValue: {instancePK: async ()=>1} },
			{ provide: OpcServerService, useValue: {server: ()=>Promise.reject(thrown)} }
		]});
		const page = TestBed.createComponent( OpcServerDetail ).componentInstance;
		page.ngOnInit();
		return page;
	};
	const settle = ()=>new Promise( r=>setTimeout(r, 0) );

	it( 'quotes a ProtoService rejection', async ()=>{
		const page = create( {error: {requestId:1, message:"no such instance", httpStatus:404}} );
		await settle();
		expect( page.error() ).toBe( "Could not load the OPC server.  (404)no such instance" );
	} );

	it( 'quotes an HttpErrorResponse', async ()=>{
		const page = create( new HttpErrorResponse({status:500, error:{message:"server exploded"}}) );
		await settle();
		expect( page.error() ).toBe( "Could not load the OPC server.  server exploded" );
	} );

	it( 'never leaves [object Object] on screen', async ()=>{
		const page = create( {error: {message:"nope"}} );
		await settle();
		expect( page.error() ).not.toContain( "[object Object]" );
		expect( page.isLoading() ).toBe( false );//the banner exists so the page does not stay blank behind isLoading
	} );
} );
