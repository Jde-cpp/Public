import { Injectable } from '@angular/core';

type Constructor<T = object> = new (...args: any[]) => T;

function factory<T>(ctor: Constructor<T>, ...args: any[]): T {
  return new ctor(...args);
}

@Injectable( { providedIn: 'root' } )
export class ProfileStore{
	constructor() {
	}
	static showDeleted( collectionName:string ):boolean{
		const item = localStorage.getItem( `${collectionName}.showDeleted` );
		let value = item ? item=="Y" : false;
		return value;
	}
	static setShowDeleted( collectionName:string, showDeleted:boolean ):void{
		if( showDeleted )
			localStorage.setItem( `${collectionName}.showDeleted`, "Y" );
		else
			localStorage.removeItem( `${collectionName}.showDeleted` );
	}
	static local<T>( key:string, defaultValue:T ):T{
		let index = key.indexOf( '/' );
		let defaultKey = index==-1 ? key : key.substring( 0, index );
		let savedDefault = ProfileStore.localDefaults.get( defaultKey );
		if( savedDefault==undefined )
			ProfileStore.localDefaults.set( defaultKey, JSON.stringify( defaultValue ) );

		const item = localStorage.getItem( key );
		if( item )
			ProfileStore.localOriginalValues.set( key, item );
		return item ? JSON.parse( item ) as T : defaultValue;
	}
	async set<T>( key:string, value:T|string|null ):Promise<void>{
		if( value!=null && typeof value != 'string' )
			value = JSON.stringify( value );
		if( value )
			localStorage.setItem( key, value as string );
		else
			localStorage.removeItem( key );
	}
	async load<T>( key:string, defaultValue:T ): Promise<T>{
		//return this.profileService.load<T>( key, defaultValue );
		return Promise.resolve( ProfileStore.local<T>(key, defaultValue) );
	}

	async loadClassArray<T>( key:string, ctor: Constructor<T>, ...args: any[] ): Promise<T[]>{
		//return this.profileService.load<T>( key, defaultValue );
		let json = ProfileStore.local<any[]>( key, [] );
		return json.map( item => factory(ctor, ...[item, ...args]) );
	}

	static pageSize( key:string ):number{
		return +(localStorage.getItem( key+".pageSize" ) ?? 24);
	}
	static setPageSize( key:string, value:number ):void{
		if( value!=24 )
			localStorage.setItem( key+".pageSize", value.toString() );
		else
			localStorage.removeItem( key+".pageSize" );
	}

	static tabIndex( key:string ):number{
		const item = localStorage.getItem( key );
		let value = item ? +item : 0;
		return value;
	}
	static setTabIndex( key:string, value:number|undefined ):void{
		if( value )
			localStorage.setItem( key, value.toString() );
		else
			localStorage.removeItem( key );
	}
	static viewIndex( collectionName:string ):number{
		const item = localStorage.getItem( `${collectionName}/viewIndex` );
		return item ? +item : 0;
	}
	static setViewIndex( collectionName:string, value:number ):void{
		if( value )
			localStorage.setItem( `${collectionName}/viewIndex`, value.toString() );
		else
			localStorage.removeItem( `${collectionName}/viewIndex` );
	}

	async save<T>( key:string, value:T|string|null ):Promise<void>{
		return Promise.resolve( this.set<T>(key, value) );
	}
	private static localDefaults:Map<string, string> = new Map<string, string>();
	//#defaults:Map<string, string> = new Map<string, string>();
	private static localOriginalValues:Map<string, string> = new Map<string, string>();
}
