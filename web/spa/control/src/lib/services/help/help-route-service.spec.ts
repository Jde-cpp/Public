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
import { ActivatedRoute, ActivatedRouteSnapshot, convertToParamMap } from '@angular/router';
import { RouteStore } from '../route-store';
import { HELP_TOPICS, HelpTopic } from './help-topic';
import { HelpRouteService, helpTopicResolver } from './help-route-service';

const overview:HelpTopic = { id: 'overview', title: 'Overview', summary: 'What this is', icon: 'menu_book', url: 'assets/site/help/overview.md' };
const gateways:HelpTopic = { id: 'gateways', title: 'Gateways', icon: 'hub', url: 'assets/jde-opc/help/gateways.md', routes: ['gateways'] };

const configure = ()=>{
	TestBed.resetTestingModule();
	TestBed.configureTestingModule({ providers: [
		HelpRouteService,
		{ provide: ActivatedRoute, useValue: {} },//RouteService injects it; nothing here reads it
		{ provide: HELP_TOPICS, useValue: [overview], multi: true },
		{ provide: HELP_TOPICS, useValue: [gateways], multi: true }
	]});
};

describe( 'HelpRouteService', ()=>{
	beforeEach( configure );

	it( 'lists the registered topics as tiles, in registration order', async ()=>{
		const items = await TestBed.inject( HelpRouteService ).docItems( [] );
		expect( items.map(i=>i.path) ).toEqual( ['overview', 'gateways'] );
		expect( items[0] ).toMatchObject( {title: 'Overview', summary: 'What this is', icon: 'menu_book', cardClass: 'card-help'} );
	});
});

describe( 'helpTopicResolver', ()=>{
	beforeEach( ()=>{ configure(); localStorage.clear(); } );

	const resolve = ( topic:string )=>TestBed.runInInjectionContext( ()=>helpTopicResolver( <ActivatedRouteSnapshot>{paramMap: convertToParamMap({topic})}, <any>{} ) );

	it( 'returns the topic for the segment and seeds the breadcrumb store', ()=>{
		expect( resolve('gateways') ).toBe( gateways );
		expect( TestBed.inject(RouteStore).getChildren('help').map(c=>c.title) ).toEqual( ['Overview', 'Gateways'] );
	});
	it( 'returns undefined for an unknown segment', ()=>expect( resolve('nope') ).toBeUndefined() );
});
