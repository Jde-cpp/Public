if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { RouteItem } from '../pages/component-sidenav/route-item';
import { RouteStore } from './route-store';

//angular-review3 L3: the cached children are the NAMES of the rows the last user browsed, and nothing dropped them at
//logout - the navbar search read them straight back out of localStorage for whoever logged in next.
describe( 'RouteStore.clear', ()=>{
	beforeEach( ()=>localStorage.clear() );

	const child = ( title:string )=>new RouteItem( {title, path:title.toLowerCase(), icon:''} );

	it( 'drops every cached url, the key list and the in-memory map', ()=>{
		const store = new RouteStore();
		store.setChildren( 'users', [child('Alice'), child('Bob')] );
		store.setChildren( 'gateways/gw1', [child('Line 1')] );
		expect( [...store.entries().keys()] ).toEqual( ['users', 'gateways/gw1'] );

		store.clear();

		expect( localStorage.getItem('users') ).toBeNull();
		expect( localStorage.getItem('gateways/gw1') ).toBeNull();
		expect( localStorage.getItem(RouteStore.keysKey) ).toBeNull();
		expect( store.entries().size ).toBe( 0 );
	} );

	it( 'leaves nothing for a freshly constructed store to hydrate', ()=>{
		const store = new RouteStore();
		store.setChildren( 'roles', [child('Admin')] );
		store.clear();
		expect( new RouteStore().getChildren('roles') ).toEqual( [] );//a new user's navbar must not see the last one's rows
	} );

	it( 'keeps going when one key cannot be removed', ()=>{
		const store = new RouteStore();
		store.setChildren( 'users', [child('Alice')] );
		store.setChildren( 'roles', [child('Admin')] );
		const removeItem = localStorage.removeItem.bind( localStorage );
		localStorage.removeItem = ( k:string )=>{ if( k=='users' ) throw new Error( "site data blocked" ); removeItem( k ); };
		try{
			expect( ()=>store.clear() ).not.toThrow();//one bad key must not strand the rest
			expect( localStorage.getItem('roles') ).toBeNull();
			expect( localStorage.getItem(RouteStore.keysKey) ).toBeNull();
		}
		finally{ localStorage.removeItem = removeItem; }
	} );
} );
