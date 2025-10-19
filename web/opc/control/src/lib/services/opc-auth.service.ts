import { Inject, Injectable, computed, signal } from '@angular/core';
import { AppService, Log } from 'jde-framework';
import { GatewayService } from './gateway.service';
import { EProvider, IAuth, User } from 'jde-spa';

@Injectable()
export class OpcAuthService implements IAuth{
	constructor( private app: AppService, @Inject('GatewayService') private gatewayService: GatewayService )
	{}

	googleAuthClientId(): Promise<string> {
		return this.app.googleAuthClientId( (m)=>console.log(m) );
	}

	async login( user:User, log:Log ):Promise<void>{
		let promise = await this.app.login( user, log );
		this.isOpc.set( false );
		return promise;
	}
	providers( log:Log ):Promise<EProvider[]>{ return this.app.providers( log ); }
	validateSessionId():void{ this.app.validateSessionId(); }

	async logout( log:Log ):Promise<void>{
		let promise;
		if( this.isOpc() )
			promise = await this.gatewayService.defaultGateway.logout( log );
		else
			promise = await this.app.logout( log );
		this.isOpc.set( null );
		return promise;
	}
	async loginPassword( domain:string, username:string, password:string, log:Log ):Promise<void>{
		//let gateway = await	 this.gatewayService.instance( domain );
		let promise = await this.gatewayService.defaultGateway.login( domain, username, password, log );
		this.isOpc.set( true );
		return promise;
	}
	user = computed( () => this.app?.user() );
		//this.isOpc()==null ? null : this.isOpc() ? this.iot.user() : this.app.user() );
	isOpc = signal<boolean|undefined>( null );
//	iot = computed( () => this.isOpc() ? this.gatewayService.defaultGateway : null );
}