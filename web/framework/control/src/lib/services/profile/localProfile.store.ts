import { Injectable } from '@angular/core';
import{ IProfileStore } from './profile.store'
import { Observable,of, throwError } from 'rxjs';

@Injectable({ providedIn: 'root' })
export class LocalProfileStore implements IProfileStore{
	static tabIndex( key:string ):number{
		const item = localStorage.getItem( key );
		let value = item ? +item : 0;
		return value;
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

	get<T>( key:string, defaultValue:T ):T{
		let index = key.indexOf( '/' );
		let defaultKey = index==-1 ? key : key.substring( 0, index );
		let savedDefault = this.#defaults.get( defaultKey );
		if( savedDefault==undefined )
			this.#defaults.set( defaultKey, JSON.stringify( defaultValue ) );

		const item = localStorage.getItem( key );
		let value = item ? JSON.parse( item ) as T : defaultValue;
		if( item )
			this.#originalValues.set( key, JSON.stringify(item) );
		return value;
	}
	load<T>( key:string, defaultValue:T ): Promise<T>{
		return Promise.resolve( this.get<T>(key, defaultValue) );
	}
	static setTabIndex( key:string, value:number ):void{
		if( value )
			localStorage.setItem( key, value.toString() );
		else
			localStorage.removeItem( key );
	}

	save<T>( key:string, value:T|string|null ):void{
		if( value!=null && typeof value != 'string' )
			value = JSON.stringify( value );
		if( value )
			localStorage.setItem( key, value as string );
		else
			localStorage.removeItem( key );
	}
	#defaults:Map<string, string> = new Map<string, string>();
	#originalValues:Map<string, string> = new Map<string, string>();
}
