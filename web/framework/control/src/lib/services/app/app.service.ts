import { Subject,Observable, tap } from 'rxjs';
import { Injectable, Inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import {Instance} from './app.service.types'

import { ETransport, ProtoService, RequestId } from '../proto.service';
import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as AppFromClient from '../../proto/App.FromClient'; import FromClient = AppFromClient.Jde.App.Proto.FromClient;
import * as AppCommon from '../../proto/App'; import App = AppCommon.Jde.App.Proto;
import * as LogProto from '../../proto/Log'; import ELogLevel = LogProto.Jde.App.Log.Proto.ELogLevel;
import * as CommonProto from '../../proto/Common'; import IException = CommonProto.Jde.Proto.IException;
import { IAuth, IEnvironment, User } from 'jde-spa';
import { IGraphQL, Log } from '../IGraphQL';
import { AuthStore } from '../auth.store';

@Injectable( {providedIn: 'root'} )
export class AppService extends ProtoService<FromClient.Transmission,FromServer.IMessage> implements IGraphQL, IAuth{
	constructor( http: HttpClient, @Inject('IEnvironment') private environment: IEnvironment, @Inject("AuthStore") authStore:AuthStore ){
		super( FromClient.Transmission, http, environment.get<ETransport>("httpTransport"), authStore, true );
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

	async gatewayInstances():Promise<Instance[]>{
		const y = await this.get( "opcGateways", (m)=>console.log(m) );
		return y["servers"];
	}

	async opcServerInstances():Promise<Instance[]>{
		const y = await this.get( "opcServers", (m)=>console.log(m) );
		return y["servers"];
	}


/*	getApplications():Promise<FromServer.IApplication[]>{
		return this.applications.length
			? Promise.resolve( this.applications )
			: new Promise<FromServer.IApplication[]>( (resolve,reject)=>
			{
				this.applicationPromises.push( new PromiseCallbacks<FromServer.IApplication[]>(resolve,reject) );
				if( this.applicationPromises.length==1 )
					this.sendRequest( FromClient.ERequest.Applications );
			});
	};
*/
	statuses():Observable<FromServer.IStatus>{
		var eventStream = new Subject<FromServer.IStatus>();
		this.statusSubscriptions.push( eventStream );
		if( this.statusSubscriptions.length==1 ){
			this.send( {subscribeStatus:true}, "Subscribe: statuses" );
		}
		return eventStream;
	}
	statusUnsubscribe( subscription:Observable<FromServer.IStatus> ){
		const index = this.statusSubscriptions.indexOf( <Subject<FromServer.IStatus>>subscription );
		if( index>-1 )
			this.statusSubscriptions.splice( index, 1 );
		if( !this.statusSubscriptions.length )
			this.send( {subscribeStatus:false}, "UnSubscribe: statuses" );
	};

	logs( applicationId:number, level:ELogLevel, start:Date, limit:number ):Observable<FromServer.ITrace>{
		const columns = "id instance_id time level message_id file_id function_id line user_pk thread_id args";
		const q = `subscribe logs(applicationId:${applicationId}, limit:${limit}, filter:{ level:{gte:${level}}, {time:{gte:${start.toISOString()}}} }){ ${columns} }`;
		const requestId = this.send( {graphQl:q}, q );
		let callback:Subject<FromServer.ITrace> = new Subject<FromServer.ITrace>();
		this.logsSubscriptions.set( requestId, callback );
		return callback.pipe(
			tap( {unsubscribe:()=>{this.logsUnsubscribe( requestId );}} )
		);
	}
	logsUnsubscribe( requestId:RequestId ){
		if( this.log.subRequest ) console.log( `[${requestId}]UnSubscribe: logs()` );
		this.logsSubscriptions.delete( requestId );
		this.sendWithId( {requestIdType:FromClient.ERequestType.UnsubscribeLogs}, requestId, "UnSubscribe: logs" );
	};

	updateLogLevel( instanceId:number, defaultFileLevel:ELogLevel, defaultDBLevel:ELogLevel ):void{
		const q = `{ mutation  LogApplicationInstances( id:${instanceId} dbLogLevel:${defaultDBLevel}, fileLogLevel:${defaultFileLevel} ){}`;
		this.send( {graphQL:q}, q );
	}

	requestStrings( strings:App.StringMD5s ):Promise<FromServer.Strings>{
		const requestId = this.send( {strings:strings}, `AppService::requestStrings count='${strings.files.length+strings.functions.length+strings.messages.length+strings.userPKs.length}'` );
		return new Promise<FromServer.Strings>( (resolve,reject)=>{ this.stringRequests.set(requestId,{resolve:resolve,reject:reject})} );
	}
/*	request( instanceId:number, value:number ){
		var request = new FromClient.RequestApp(); request.type = value; request.instanceId = instanceId;
		var msg = new FromClient.MessageUnion(); msg.requestApp=request;
		this.send( msg );
	}
*/
	custom( appPK:number, bytes:Uint8Array ):Promise<Uint8Array>{
		const requestId = this.send( {forwardExecution:{appPK:appPK, executionTransmission:bytes}}, `custom appPK: ${appPK}, bytes: ${bytes.length}` );
		return new Promise<Uint8Array>( (resolve,reject)=>{ this.customCallbacks.set(requestId,{resolve:resolve,reject:reject})} );
	}

/*	private sendRequest( x:FromClient.ERequest ){
		console.log( `requesting '${FromClient.ERequest[x]}'`)
		let msg = { request: {type:x} };
		this.send( msg );
	}

	private sendSingularRequest( x:FromClient.ERequest ):Promise<string>{
		const m = { value:x, instanceId:super.nextRequestId };
		let p = new Promise<string>( (resolve,reject)=>{
			this.singularRequests.set( m.instanceId, {resolve:resolve,reject:reject} );
		});

		console.log( `(${m.instanceId})requesting '${FromClient.ERequest[m.value]}'` );
		this.send( m );
		return p;
	}
*/
	async login( user:User, log:Log ):Promise<void>{
		let self = this;
		//if( this.log.restRequests )	log( `googleLogin( ${user.credential} )` );
		console.assert( !user.sessionId );
		user.sessionId = await super.loginJwt( user.authorization );
		self.authStore.append( user );
		//if( this.log.restResults )	log( `authorization='${self.authorization}'` );
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

	private logsSubscriptions:Map<RequestId,Subject<FromServer.ITrace>>= new Map<RequestId,Subject<FromServer.ITrace>>();
	//private addMessage( msg ):void{}
	override handleConnectionError( err ):void{
		this.statusSubscriptions.forEach( (x)=>x.error( err ) );
		this.statusSubscriptions = [];
	}
	encode( t:FromClient.Transmission ){ return FromClient.Transmission.encode(t); }
	public async validateSessionId():Promise<User | null>{
		console.log( `validateSessionId: ${this.user()?.authorization}` );
		if( !this.user() )
			return Promise.resolve( null );
		const y = await this.query<{session:{domain:string,loginName:string}}>( `session( id:"${this.user().sessionId}" ){ domain loginName }` );
		return new User( { domain:y.session.domain, id:y.session.loginName, sessionId:this.user().sessionId } );
	}
	private complete():void{
		console.log( 'complete' );
		for( const subscription of this.statusSubscriptions )
			subscription.complete();
	}

	protected processMessage( bytearray:protobuf.Buffer ){
		try{
			let t:FromServer.Transmission;
			try{
				t = FromServer.Transmission.decode( bytearray );
			}
			catch( e ){
				throw `error decode ${bytearray.length} bytes, error: ${JSON.stringify(e)}`;
			}
			for( const message of t.messages ){
				const requestId = message.requestId;
				if( super.processCommonMessage(message, requestId) )
					continue;
				if( message.ack ){//first message after handshake
					console.log( `[App.${requestId}]Connected to '${super.socketUrl}', socketId: ${message.ack}` );
					let socketId = message.ack;
					if( !this.user() )
						super.sendAuthorization( socketId );
					else{
						console.warn( `no authorization` );
						this.setSocketId( socketId );
					}
				}
				else if( message.executeResponse ){
					var promise = this.customCallbacks.get( requestId ); if( !promise ) throw `no promise for requestId=${requestId}`;
					promise.resolve( message.executeResponse );
				}
				else if( message.status ){
					if( this.log.subResults )	console.log( `[App.${requestId}]status appId:${message.status.applicationId}` );
					for( const callback of this.statusSubscriptions )
						callback.next( message.status );
				}
				else if( message.strings ){
					const x = message.strings;
					if( this.log.sockResults ) console.log( `[App.${requestId}]strings messageCount: ${Object.keys(x.messages).length}` );
					let promise = this.stringRequests.get( requestId ); if( !promise ) throw `no promise for requestId=${requestId}`;
					promise.resolve( x );
				}
				else if( message.traces ){
					if( this.log.subResults )	console.log( `[App.${requestId}]traces count:${message.traces.values.length}` );
					let subject = this.logsSubscriptions.get( requestId ); if( !subject ) throw `no subscription for requestId=${requestId}`;
					message.traces.values.forEach( x=>subject.next(x) );
				}
				else if( message.exception ){
					let processed = false;
					if( requestId ){
						processed = super.processError(message.exception, requestId)
							|| this.iotProcessError(message.exception, requestId);
					}
					if( !processed )
						throw `[App.${requestId}]Error:  (${message.exception.code.toString(16)})${message.exception.what}`;
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
			this.stringRequests.get( requestId ).reject( e );
			this.stringRequests.delete( requestId );
		}
		else if( this.customCallbacks.has(requestId) ){
			this.customCallbacks.get( requestId ).reject( e );
			this.customCallbacks.delete( requestId );
		}
		else
			processed = false;
		return processed;
	}
	//private applications:FromServer.IApplication[]=[];
	//private applicationPromises:PromiseCallbacks<FromServer.IApplication[]>[]=[];
	//private singularRequests = new Map<number,{resolve:any,reject:any}>();
	private stringRequests = new Map<number,{resolve:any,reject:any}>();
	//private stringRequests:Map<FromClient.IRequestAppString,Subject<[number,FromServer.IApplicationString]>>= new Map<FromClient.IRequestAppString,Subject<[number,FromServer.IApplicationString]>>();
	private statusSubscriptions:Subject<FromServer.IStatus>[]=[];
	private customCallbacks = new Map<number,{resolve:any,reject:any}>();
	//private stringValuePromises = new Map<number,Promise<string>>();
	//private _customRequestId:number=0;
//	private static Url = environment.applicationServerUrl;// 'ws://localhost:1967';
}