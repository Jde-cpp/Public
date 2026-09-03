//the vitest environment provides window/document but no localStorage - back the bare-global references with an
//in-memory one BEFORE importing jde-spa (see google-relogin.spec.ts).
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
import { provideRouter, Router, Routes } from '@angular/router';
import { RouteItem, RouteSearchProvider, RouteStore } from 'jde-spa';

class Dummy{}
const routes:Routes = [
	{ path: '', title: 'Home', component: Dummy },
	{ path: 'login', component: Dummy, data: {name: 'Login'} },
	{ path: 'gateways', title: 'Gateways', component: Dummy, data: {summary: 'Available Gateways', icon: 'hub'} },
	{ path: 'gateways/:gateway', title: ':gateway', component: Dummy },
	{ path: 'gateways/:gateway/:connection', component: Dummy, children: [ { path: '**', component: Dummy } ] },
	{ path: 'access', title: 'Access', component: Dummy, data: {summary: 'Configure User Access', icon: 'admin_panel_settings'} },
	{ path: 'access', component: Dummy, children: [
		{ path: 'users/:target', component: Dummy },
		{ path: ':collectionDisplay', component: Dummy, data: { collections: [ {path: 'users', data: {icon: 'person'}}, 'roles' ] } },
	]},
	{ path: 'apps', title: 'Applications', component: Dummy },
	{ path: 'apps/gateways/:instance', component: Dummy, children: [ { path: '', component: Dummy }, { path: ':connection', component: Dummy } ] },
];

describe('RouteSearchProvider', () => {
	const stored = new Map<string,RouteItem[]>();
	let provider:RouteSearchProvider;
	beforeEach( () => {
		stored.clear();
		TestBed.configureTestingModule({ providers: [ provideRouter(routes), { provide: RouteStore, useValue: { entries: ()=>stored } } ] });
		provider = TestBed.inject( RouteSearchProvider );
	});

	it('walks the static tree: titles, data, the collections behind :collectionDisplay; skips login, params, ** and duplicates', () => {
		const items = provider.items();
		const byRoute = new Map( items.map( i=>[i.route as string, i] ) );
		expect( [...byRoute.keys()].sort() ).toEqual( ['/', '/access', '/access/roles', '/access/users', '/apps', '/gateways'] );
		expect( byRoute.get('/gateways') ).toMatchObject( {title: 'Gateways', icon: 'hub', summary: 'Available Gateways'} );
		expect( byRoute.get('/access/users') ).toMatchObject( {title: 'Users', icon: 'person'} );
		expect( byRoute.get('/access/roles')?.title ).toBe( 'Roles' );
	});

	//the dynamic hits navigate by COMMANDS ARRAY, raw segments - router.navigate encodes each one, and a pre-encoded string
	//double-encoded them (angular-review3 #7).  The encoded url survives as the dedup key only.
	it('rebuilds absolute urls from every RouteStore key style the writers use', () => {
		stored.set( 'gateways/gw1', [ new RouteItem({title: 'Line 1', path: 'cn1'}) ] );//GatewayCnnctnRouteService: joined UrlSegments
		stored.set( '/apps', [ new RouteItem({title: 'Gateway/Debug', path: 'gateways/Debug'}) ] );//Apps page: leading slash, child carries its section
		stored.set( 'apps/gateways', [ new RouteItem({title: 'Debug', path: 'gateways/Debug'}) ] );//AppResolver: child repeats the key's tail
		stored.set( 'users', [ new RouteItem({title: 'Alice', path: 'alice'}) ] );//QLListResolver: bare collection name
		stored.set( 'nowhere', [ new RouteItem({title: 'Lost', path: 'x'}) ] );//no route matches - dropped
		const items = provider.items();
		const find = ( title:string )=>items.find( i=>i.title==title );
		expect( find('Line 1') ).toMatchObject( {route: ['/gateways', 'gw1', 'cn1'], summary: 'gateways / gw1'} );
		expect( find('Gateway/Debug')?.route ).toEqual( ['/apps', 'gateways', 'Debug'] );
		expect( find('Debug') ).toBeUndefined();//same url as the Apps-page entry, which came first
		expect( find('Alice')?.route ).toEqual( ['/access', 'users', 'alice'] );
		expect( find('Lost') ).toBeUndefined();
	});

	//review3 #7: every Google-provisioned target is '<provider>-<email>'.  Pre-encoding gave the router '%40', which it
	//encoded again to '%2540', so paramMap yielded '…%40…', targetQuery matched no row and the page bounced.
	it('hands the router raw segments, so an @ target survives navigation', async () => {
		stored.set( 'users', [ new RouteItem({title: 'John', path: 'Google-johnmduffy@gmail.com'}) ] );
		const item = provider.items().find( i=>i.title=='John' )!;
		expect( item.route ).toEqual( ['/access', 'users', 'Google-johnmduffy@gmail.com'] );
		const router = TestBed.inject( Router );
		await router.navigate( item.route as any[] );
		expect( router.url ).toBe( '/access/users/Google-johnmduffy@gmail.com' );//NOT %2540
		expect( decodeURIComponent(router.url.split('/').pop()!) ).toBe( 'Google-johnmduffy@gmail.com' );
	});

	it('ranks title starts-with ahead of contains, ignores scoped queries', async () => {
		stored.set( 'gateways', [ new RouteItem({title: 'Plant gateway', path: 'plant'}) ] );
		const results = await provider.search( 'gate', undefined, 10 );
		expect( results.map(r=>r.title) ).toEqual( ['Gateways', 'Plant gateway'] );
		expect( await provider.search('gate', 'user', 10) ).toEqual( [] );
		expect( await provider.search('', undefined, 10) ).toEqual( [] );
	});
});
