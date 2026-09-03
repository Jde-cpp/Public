import { Subject,Observable, tap } from 'rxjs';
import { Injectable, Inject, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import {Instance} from './app-service-types'

import { ETransport, ProtoService, RequestId } from '../proto-service';
import * as FromServer from 'jde-proto/App.FromServer';
import * as FromClient from 'jde-proto/App.FromClient';
import * as App from 'jde-proto/App';
import { ELogLevel } from 'jde-proto/Log';
import { Exception as IException } from 'jde-proto/Common';
import { IAuth, IEnvironment, User } from 'jde-spa';
import { IGraphQL, Log } from '../graphql';
import { AuthStore } from '../auth-store';
import { GoogleAuthService } from '../google-auth-service';
import { StringUtils } from '../../utils/string-utils';

@Injectable( {providedIn: 'root'} )
export class AppService extends ProtoService<FromClient.Transmission,FromServer.Message> implements IGraphQL, IAuth{
	constructor( http: HttpClient, @Inject('IEnvironment') private environment: IEnvironment, @Inject("AuthStore") authStore:AuthStore ){
		//?? Unsecure:  the key was missing from both environment files, so this passed `undefined` as the transport.  Every
		//test in ProtoService is `==Secure`/`==Hybrid`, which undefined fails, so it behaved as Unsecure by accident - but
		//`transport==ETransport.Unsecure` was FALSE too, since that enumerator is 0.  Default it to a real enumerator.
		super( FromClient.Transmission, http, environment.get<ETransport>("httpTransport") ?? ETransport.Unsecure, authStore, true, inject(GoogleAuthService) );
		let appServer = environment.get<Instance>( 'applicationServer' );
		if( !appServer ){
			console.log( "No Application Server set in environment" );
			appServer = { port:1967, host:"localhost" };
		}
		console.log( `AppService: ${appServer.host}:${appServer.port}` );
		super.instances = [appServer];
	}
	// ping():Promise<string>{
	// 	return this.sendSingularRequest( FromClient.ERequest.Ping );
	// }

	//App.FromClient's session_id is a `uint32` - a session PK the AppServer turns straight back into the session key with
	//{:x} - while the browser holds it as the hex string the Authorization response header carried.  So it has to go back as
	//a NUMBER;  the raw string threw @bufbuild's assertUInt32 and left the socket unauthenticated (see the base method).
	//A user holding only a jwt has no session to adopt yet:  Message.jwt is a DELEGATED validation (ServerSocketSession::Login
	//echoes the answer and installs nothing on the socket), not a handshake, so there is nothing to send.
	protected override authorizationMessage( authorization:string ):any|undefined{
		const sessionId = /^[0-9a-fA-F]+$/.test( authorization ) ? parseInt( authorization, 16 ) : NaN;//parseInt alone stops at the first bad digit, so "Bearer ey..." would have become NaN but "1a2bZZ" a plausible-looking 6699
		if( !Number.isSafeInteger(sessionId) || sessionId<=0 || sessionId>0xFFFFFFFF ){
			console.warn( `AppService.authorizationMessage: '${authorization}' is not a uint32 hex session id.` );
			return undefined;
		}
		return { sessionId };
	}

	async gatewayInstances():Promise<Instance[]>{
		const y = await this.get( "opcGateways", (m)=>console.log(m) ) as any;
		return y["servers"];
	}

	async opcServerInstances():Promise<Instance[]>{
		const y = await this.get( "opcServers", (m)=>console.log(m) ) as any;
		return y["servers"];
	}

	async instancePK( instanceName:string, programName?:string ):Promise<number|undefined>{
		const connections = await this.queryArray<{id?:number, instanceId?:number, instanceName:string, programName:string}>( "connections{instanceId instanceName programName}", null, (m)=>console.log(m) );
		const strip = ( p:string )=> p?.startsWith("Jde.") ? p.substring(4) : p;
		const match = connections.find( c=>c.instanceName==instanceName && (!programName || strip(c.programName)==strip(programName)) );//instance names are only unique per program — "Debug" exists for both the AppServer and the gateway
		return match?.instanceId ?? match?.id;//servers whose connections view predates the instanceId column emit the instance pk under 'id'
	}

	//BROKEN, same class as requestStrings below (angular-review3 L10):  App.FromClient.proto has no `graph_ql` field, so both
	//`{graphQl:…}` here and `{graphQL:…}` in updateLogLevel encode to a requestId-only message.  Unlike the two renames
	//nearby the fix is not a rename - it is `subscription`/`query`, whose type is a Jde.Proto.Query {text variables
	//returnRaw} - and neither method has an in-repo caller to verify the choice against, so they are left flagged.
	logs( applicationId:number, level:ELogLevel, start:Date, limit:number ):Observable<FromServer.Trace>{
		const columns = "id instance_id time level message_id file_id function_id line user_pk thread_id args";
		const q = `subscribe logs(applicationId:${applicationId}, limit:${limit}, filter:{ level:{gte:${level}}, {time:{gte:${start.toISOString()}}} }){ ${columns} }`;
		const requestId = this.send( {graphQl:q}, q );
		let callback:Subject<FromServer.Trace> = new Subject<FromServer.Trace>();
		this.logsSubscriptions.set( requestId, callback );
		return callback.pipe(
			tap( {unsubscribe:()=>{this.logsUnsubscribe( requestId );}} )
		);
	}
	logsUnsubscribe( requestId:RequestId ){
		if( this.log.subRequest ) console.log( `[${requestId}]UnSubscribe: logs()` );
		this.logsSubscriptions.delete( requestId );
		this.sendWithId( {requestType:FromClient.ERequestType.UnsubscribeLogs}, requestId, "UnSubscribe: logs" );//`requestType`, per App.FromClient.proto's `ERequestType request_type = 14` - see requestStrings below
	};

	updateLogLevel( instanceId:number, defaultFileLevel:ELogLevel, defaultDBLevel:ELogLevel ):void{
		const q = `{ mutation  LogApplicationInstances( id:${instanceId} dbLogLevel:${defaultDBLevel}, fileLogLevel:${defaultFileLevel} ){}`;
		this.send( {graphQL:q}, q );
	}

	//`requestStrings`, not `strings`:  App.FromClient.proto declares `StringMD5s request_strings = 10` and there is no
	//`strings` field, so ts-proto's encode() dropped the payload and put a requestId-only message on the wire.  The server
	//has nothing to answer, so the promise below never settled and the log rows kept their raw ids forever.
	requestStrings( strings:App.StringMD5s ):Promise<FromServer.Strings>{
		const requestId = this.send( {requestStrings:strings}, `AppService::requestStrings count='${strings.files.length+strings.functions.length+strings.messages.length+strings.userPKs.length}'` );
		return new Promise<FromServer.Strings>( (resolve,reject)=>{ this.stringRequests.set(requestId,{resolve:resolve,reject:reject})} );
	}

	custom( appPk:number, bytes:Uint8Array ):Promise<Uint8Array>{
		const requestId = this.send( {forwardExecution:{appPk:appPk, executionTransmission:bytes}}, `custom appPk: ${appPk}, bytes: ${bytes.length}` );
		return new Promise<Uint8Array>( (resolve,reject)=>{ this.customCallbacks.set(requestId,{resolve:resolve,reject:reject})} );
	}

	async login( user:User, log:Log ):Promise<void>{
		console.assert( !user.sessionId );
		await super.loginJwt( user.authorization! );//the sessionId arrives in the response Authorization header — postRaw already appended it to authStore; the /login body is metadata ({expiration}), NOT the sessionId
		this.authStore.append( user );//persist the jwt/identity fields; user.sessionId stays undefined so append() keeps the header-derived sessionId instead of clobbering it with the body
	}
	loginPassword( username:string, password:string, authenticator:string ):Promise<void>{
		throw "noImpl";
	}
	async logout( log:Log ):Promise<void>{
		let self = this;
		if( this.log.restRequests )	log( `logout()` );
		const options = { observe: "response", headers:{"Authorization":this.user()?.authorization} };
		let result = await this.postRaw<string>( 'logout', {}, false, options );
		self.authStore.logout();
		if( this.log.restResults ) log( `logout=>${result}` );
	}

	async googleAuthClientId( log:Log ):Promise<string>{
		return await super.querySetting( "googleAuthClientId", log );
	}

	private logsSubscriptions:Map<RequestId,Subject<FromServer.Trace>>= new Map<RequestId,Subject<FromServer.Trace>>();
	//private addMessage( msg ):void{}
	//the base settles _callbacks; these three maps are AppService's own pending work and would otherwise hang forever when the socket drops.
	override handleConnectionError( err:unknown ):void{
		const e = { message: "Connection to the application server was lost." };
		const strings = [...this.stringRequests.values()]; this.stringRequests.clear();//drain-then-settle: a handler may issue a fresh request, and it must not land in the map being cleared
		strings.forEach( p=>p.reject(e) );
		const customs = [...this.customCallbacks.values()]; this.customCallbacks.clear();
		customs.forEach( p=>p.reject(e) );
		const subscriptions = [...this.logsSubscriptions.values()]; this.logsSubscriptions.clear();//clear before error() so a resubscribe starts a fresh entry, per the gateway's clearOwner precedent
		subscriptions.forEach( s=>s.error(e) );
	}
	public async validateSessionId():Promise<User | null>{
		console.log( `validateSessionId: ${this.user()?.authorization}` );
		let user = this.user();
		if( !user )
			return Promise.resolve( null );
		const y = await this.query<{session:{domain:string,loginName:string}}>( `session( id:${StringUtils.qlString(user.sessionId!)} ){ domain loginName }` );
		return new User( { domain:y.session.domain, id:y.session.loginName, sessionId:user.sessionId } );
	}
	private complete():void{
		console.log( 'complete' );
	}

	protected processMessage( bytearray:Uint8Array ){
		try{
			let t:FromServer.Transmission;
			try{
				t = FromServer.Transmission.decode( bytearray );
			}
			catch( e ){
				throw `error decode ${bytearray.length} bytes, error: ${JSON.stringify(e)}`;
			}
			for( const message of t.messages ){
				const requestId = message.requestId as number;
				if( super.processCommonMessage(message, requestId) )
					continue;
				if( message.ack ){//first message after handshake
					console.log( `[App.${requestId}]Connected to '${super.socketUrl}', socketId: ${message.ack}` );
					let socketId = message.ack;
					if( this.user()?.authorization )//not `this.user()`: logout() leaves a truthy serverInstances-only User whose authorization is null
						super.sendAuthorization( socketId );
					else{
						console.warn( `no authorization` );
						this.setSocketId( socketId );
					}
				}
				else if( message.executeResponse ){
					var promise = this.customCallbacks.get( requestId ); if( !promise ) throw `no promise for requestId=${requestId}`;
					this.customCallbacks.delete( requestId );//settled requests must be removed or the map grows for the socket's lifetime
					promise.resolve( message.executeResponse );
				}
				else if( message.strings ){
					const x = message.strings;
					if( this.log.sockResults ) console.log( `[App.${requestId}]strings messageCount: ${Object.keys(x.messages as any).length}` );
					let promise = this.stringRequests.get( requestId ); if( !promise ) throw `no promise for requestId=${requestId}`;
					this.stringRequests.delete( requestId );
					promise.resolve( x );
				}
				else if( message.traces ){
					if( this.log.subResults )	console.log( `[App.${requestId}]traces count:${message.traces.values!.length}` );
					let subject = this.logsSubscriptions.get( requestId ); if( !subject ) throw `no subscription for requestId=${requestId}`;
					message.traces.values!.forEach( x=>subject.next(x) );
				}
				else if( message.exception ){
					let processed = false;
					if( requestId ){
						processed = super.processError(message.exception, requestId)
							|| this.iotProcessError(message.exception, requestId);
					}
					if( !processed )
						throw `[App.${requestId}]Error:  (${message.exception.statusCode})(${message.exception.code!.toString(16)})${message.exception.what}`;
				}
				else
					throw `unknown message type ${Object.keys(message)}`
			}
		}
		catch( e ){
			console.error( e );
		}
		return bytearray;
	}
	iotProcessError( e:IException, requestId:RequestId ):boolean{
		let processed = true;
		if( this.stringRequests.has(requestId) ){
			this.stringRequests.get( requestId )!.reject( e );
			this.stringRequests.delete( requestId );
		}
		else if( this.customCallbacks.has(requestId) ){
			this.customCallbacks.get( requestId )!.reject( e );
			this.customCallbacks.delete( requestId );
		}
		else
			processed = false;
		return processed;
	}
	private stringRequests = new Map<number,{resolve:any,reject:any}>();
	private customCallbacks = new Map<number,{resolve:any,reject:any}>();
}