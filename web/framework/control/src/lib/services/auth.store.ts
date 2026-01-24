import { Injectable, Signal, signal } from '@angular/core';
import { User, UserJson } from 'jde-spa';
import { clone } from '../utils/utils'

const userStorageKey = 'user';

@Injectable({ providedIn: 'root' })
export class AuthStore{
	constructor(){
		let userString = localStorage.getItem(userStorageKey);
		if( this.log ) console.log( `AuthService User: ${userString}` );
		if( userString ){
			let juser = JSON.parse( userString );
			let reinit = false;
			if( reinit ){
				juser.jwt = "ey...";
				juser.sessionId = null;
			}
			let user = new User( juser );
			this.#userSignal.set( user );
		}
	}

	append( user:UserJson ):void{
		let current = this.user() ? new User(this.user()) : new User();
		current.append( user );
		let stringify = JSON.stringify( current );
		if( this.log ) console.log( `auth.append( ${stringify} )` );
		localStorage.setItem( userStorageKey, stringify );
		this.#userSignal.set( current );
	}

	reset( serverInstance?:{url:string,instance:number}, jwt?:string ):void{
		let user:User = null;
		if( serverInstance ){
			user = new User( jwt );
			user.serverInstances = user.serverInstances || [];
			AuthStore.upsertServerInstance( user, serverInstance.url, serverInstance.instance );
		}
		if( this.log ) console.log( `auth.reset( ${user.toString()} )` );
		if( user )
			localStorage.setItem( userStorageKey, JSON.stringify(user) );
		else
			localStorage.removeItem( userStorageKey );
		this.#userSignal.set( user );
	}

	setServerInstance( url: string, instance: number ){
		let user = this.user() ?? null;
		AuthStore.upsertServerInstance( user, url, instance );
		if( this.log ) console.log( `setServerInstance( ${JSON.stringify(user)} )` );
		this.#userSignal.set( user );
	}

	static upsertServerInstance( user: User, url: string, instance: number ){
		let index = user.serverInstances.findIndex( (x)=>x.url==url );
		if( index>=0 )
			user.serverInstances[index].instance = instance;
		else
			user.serverInstances.push({ url, instance });
	}

	logout(){
		let newAuth = new User( {serverInstances:this.user()?.serverInstances} );
		if( newAuth.serverInstances ){
			localStorage.setItem( userStorageKey, JSON.stringify(newAuth) );
		}
		else
			localStorage.removeItem( userStorageKey );
		if( this.log ) console.log( `logout(${JSON.stringify(newAuth)})` );
		this.#userSignal.set( newAuth );
	}

	log:boolean = false;
	#userSignal = signal<User | null>( null );
	get user():Signal<User | null>{ return this.#userSignal.asReadonly(); }
}