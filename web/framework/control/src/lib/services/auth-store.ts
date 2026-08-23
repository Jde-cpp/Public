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

	//Drop the session, keeping only the jwt identity (undefined for password/OpcServer logins, which yields an empty
	//User exactly as before).  The old signature took a {url,instance} whose only source was loginWait's read of a
	//serverSettings key the server does not send; with that gone nothing produces an instance, so the parameter and the
	//upsert it drove went with it.
	reset( jwt?:string ):void{
		const user = new User( jwt );
		if( this.log ) console.log( `auth.reset( ${user.toString()} )` );
		localStorage.setItem( userStorageKey, JSON.stringify(user) );
		this.#userSignal.set( user );
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