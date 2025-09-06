import { Signal } from '@angular/core';

export enum EProvider{
	None = 0,
	Google=1,
	Facebook=2,
	Amazon=3,
	Microsoft=4,
	VK=5,
	Key = 6,
	OpcServer = 7
};
export class UserJson{
	constructor(){
		this.serverInstances = [];
	}
  id?: string; //loginName
	sessionId?: string;
	jwt?: string;
	domain?: string;
	exp?: Date;	//expiration
  email?: string;
	name?: string;
  picture?: string;
	provider?: EProvider;
	serverInstances?:{url:string,instance:number}[];
}
export class User extends UserJson {
	constructor( arg?:UserJson|string ){
		super();
		this.jwt = typeof arg === 'string' ? <string>arg : arg ? (<UserJson>arg).jwt : null;
		if( this.jwt ){
			const jwt = User.decodeJwt( this.jwt );
			this.email = jwt.email;
			this.name = jwt.name;
			this.picture = jwt.picture;
			this.exp = new Date( jwt.exp * 1000 );
			if( jwt.iss=="https://accounts.google.com" )
				this.provider = EProvider.Google;
		}
		if( arg && !this.jwt ){
			let json = arg as UserJson;
			this.sessionId = json.sessionId;
			this.serverInstances = json.serverInstances;
		}
	}

	append( updated:UserJson ):void{
		for( let key in updated ){
			if( updated[key] !== undefined )
				this[key] = updated[key];
		}
	}

	static decodeJwt(idToken: string):any{
		const base64Url = idToken.split( "." )[1];
		const base64 = base64Url.replace( /-/g, "+" ).replace( /_/g, "/" );
		const jsonPayload = decodeURIComponent(
			window.atob( base64 ).split("")
				.map( (c)=>"%" + ("00" + c.charCodeAt(0).toString(16)).slice(-2) )
				.join("")
		);
		return JSON.parse( jsonPayload );
	}
	override toString():string{
		if( this.jwt )
			return JSON.stringify( {sessionId:this.sessionId, jwt:this.jwt, serverInstances: this.serverInstances} );
		else
			return JSON.stringify( this );
	}
	get authorization(){ return this.sessionId ?? (this.jwt ? `Bearer ${this.jwt}` : null); }
}
type Log = (m:string)=>void;
export interface IAuth{
	login( user:User, log:Log ):Promise<void>;
	loginPassword( username:string, password:string, authenticator:string, log:Log ):Promise<void>;
	logout( log:Log ):Promise<void>;
	providers( log:Log ):Promise<EProvider[]>;
	validateSessionId( log:Log ):void;
	googleAuthClientId( log:Log ):Promise<string>;
	user:Signal<User | null>;
}