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
		let current = this.user() ? new User(this.user() as UserJson) : new User();
		current.append( user );
		let stringify = JSON.stringify( current );
		if( this.log ) console.log( `auth.append( ${stringify} )` );
		localStorage.setItem( userStorageKey, stringify );
		this.#userSignal.set( current );
	}

	reset( serverInstance?:{url:string,instance:number}, jwt?:string ):void{
		let user:User|undefined;
		if( serverInstance ){
			user = new User( jwt );
			user.serverInstances = user.serverInstances || [];
			AuthStore.upsertServerInstance( user, serverInstance.url, serverInstance.instance );
		}
		if( this.log ) console.log( `auth.reset( ${user ? user.toString() : "undefined"} )` );
		if( user )
			localStorage.setItem( userStorageKey, JSON.stringify(user) );
		else
			localStorage.removeItem( userStorageKey );
		this.#userSignal.set( user );
	}

	setServerInstance( url: string, instance: number ){
		const current = this.user();
		if( !current )
			return;//nothing to hang an instance off - append()/reset() are what create the User
		//NOT `new User(current)`: the ctor's jwt branch skips its append(), so copying a Google user that way silently drops sessionId/serverInstances (the angular-review.md #21/#36 residual).
		const user = new User();
		user.append( current );
		user.serverInstances = (user.serverInstances ?? []).map( x=>({...x}) );//own array, so the upsert below cannot reach back into the instance the signal still holds
		AuthStore.upsertServerInstance( user, url, instance );
		const stringify = JSON.stringify( user );
		if( this.log ) console.log( `setServerInstance( ${stringify} )` );
		localStorage.setItem( userStorageKey, stringify );//was never persisted, so the instance was lost on reload and loginWait's `previousInstance!=instance` re-fired every session
		this.#userSignal.set( user );//a NEW reference: signals compare with Object.is, so mutating the held instance and re-setting it notified nobody
	}

	static upsertServerInstance( user: User, url: string, instance: number ){
		let index = user.serverInstances!.findIndex( (x)=>x.url==url );
		if( index>=0 )
			user.serverInstances![index].instance = instance;
		else
			user.serverInstances!.push({ url, instance });
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
	#userSignal = signal<User | undefined>( undefined );
	get user():Signal<User | undefined>{ return this.#userSignal.asReadonly(); }
}