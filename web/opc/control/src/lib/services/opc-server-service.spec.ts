//the vitest environment provides window/document but no localStorage - back the bare-global references with an
//in-memory one BEFORE importing jde-spa/jde-framework (see the site specs).
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
import { provideHttpClient } from '@angular/common/http';
import { AppService, AUTH_STORE, AuthStore, ETransport } from 'jde-framework';
import { OpcServerService } from './opc-server-service';

const instances = [ {host:'localhost', port:1970, instanceName:'opc1'} ];

//review3 L5: `#servers ??= …` cached whatever the first call produced, INCLUDING a rejection - one dropped request left
//every OpcServer page broken until the browser reloaded.  AccessService.resources already had the .catch reset.
describe( 'OpcServerService.servers', ()=>{
	const configure = ( opcServerInstances:()=>Promise<any[]> )=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({ providers: [
			provideHttpClient(),
			{ provide: AUTH_STORE, useValue: {user: ()=>undefined, logout: ()=>{}} as unknown as AuthStore },
			{ provide: AppService, useValue: {transport: ETransport.Unsecure, opcServerInstances} }
		]});
		return TestBed.inject( OpcServerService );
	};

	it( 'retries after a failed lookup instead of replaying the rejection', async ()=>{
		let calls = 0;
		const service = configure( ()=>{ return ++calls==1 ? Promise.reject(new Error("network")) : Promise.resolve(instances); } );

		await expect( service.servers() ).rejects.toThrow( "network" );
		const servers = await service.servers();//the retry the user gets by navigating back to the page
		expect( servers.map(s=>s.name) ).toEqual( ['opc1'] );
		expect( calls ).toBe( 2 );
	} );

	it( 'still shares one round trip across concurrent callers', async ()=>{
		let calls = 0;
		const service = configure( ()=>{ ++calls; return Promise.resolve(instances); } );

		const [a, b] = await Promise.all( [service.servers(), service.servers()] );
		expect( a ).toBe( b );
		expect( calls ).toBe( 1 );
	} );

	it( 'reports a stopped server as a miss, not an empty list', async ()=>{
		const service = configure( ()=>Promise.resolve(instances) );
		await expect( service.server('nosuch') ).rejects.toThrow( /No OpcServer instance named 'nosuch'/ );
	} );
} );
