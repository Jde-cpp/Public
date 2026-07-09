import { webSocket, WebSocketSubject } from 'rxjs/webSocket';
import { firstValueFrom } from 'rxjs';
import { HttpClient, HttpEvent, HttpResponse, HttpSentEvent } from '@angular/common/http';
import { FieldKind } from '../model/ql/schema/Field';
import { fromIsoDuration, verify } from '../utils/utils';
import { TableSchema } from '../model/ql/schema/TableSchema';
import { EnumValue, Log, IQueryResult, Query } from '../services/IGraphQL';
import { MutationSchema } from '../model/ql/schema/MutationSchema';
import { Instance } from './app/app.service.types';
import * as LogProto from '../proto/Log'; import ELogLevel = LogProto.Jde.App.Log.Proto.ELogLevel;
import * as CommonProto from '../proto/Common'; import IException = CommonProto.Jde.Proto.IException;
import { AuthStore } from './auth.store';
import { Mutation } from '../model/ql/Mutation';
import { computed, Signal } from '@angular/core';
import { EProvider, User } from 'jde-spa';

interface IStringResult{ id:number; value:string; }
export interface IError{ requestId?:number; message: string; sc?:number; }

type TransformInput = (x:any)=>any;
type Resolve = (x:any)=>void;
type Reject = ( e:{error:IError} )=>void;
export type RequestId = number;
export enum ETransport{ Unsecure, Secure, Hybrid };

class RequestPromise<ResultMessage>{
	constructor( public result:undefined|((arg:ResultMessage)=>any), public resolve:Resolve, public reject:Reject, public transformInput:TransformInput|null=null )
	{}
}

export abstract class ProtoService<Transmission,ResultMessage>{
	constructor( private TCreator: { new (): Transmission; }, protected http: HttpClient, public readonly transport:ETransport, protected authStore:AuthStore, private isAppServer:boolean=false )
	{}

	connect():void{
		this.#socket = webSocket<protobuf.Buffer>( {url: this.socketUrl, deserializer: msg => this.onMessage(msg), serializer: msg=>msg, binaryType:"arraybuffer"} );
		this.#socket.subscribe(
			( msg ) => this.addMessage( msg ),
			( err ) => this.error( err ),
			() => this.socketComplete()
		);
	}
	//should deserialize put into a constant variable or process in deserialization?
	addMessage( msg:any )
	{}

	toCollectionName( collectionDisplay:string ):string{ return collectionDisplay; }
	subQueries( typeName: string, id: number ):string[]{ return []; }
	targetQuery( schema: TableSchema, target: string, showDeleted:boolean, excludedColumns:string[] ):string{
		let fields = this.fieldColumns( schema, showDeleted, excludedColumns );
		return `${schema.singular}( target:"${target}" ){ ${fields.join(" ")} }`;
	}
	protected fieldColumns( schema: TableSchema, showDeleted:boolean, excludedColumns:string[] ):string[]{
		let columns = [];
		let filtered = schema.fields.filter(
			(x)=>!excludedColumns.includes(x.name) && (x.name!="deleted" || showDeleted) );
		for( const field of filtered ){
			if( field.type.underlyingKind==FieldKind.UNION )
				columns.push( `${field.name}{id}` );
			else if( field.type.underlyingKind==FieldKind.OBJECT )
				columns.push( `${field.name}{id name}` );
			else
				columns.push( field.name );
		}
		return columns;
	}

	//
	error( err:any ){
		this.#socket = undefined;//drop the dead socket so the next send reconnects
		this.setSocketId( 0 );
		console.log( "No longer connected to Server.", err );
		this.handleConnectionError( err );
	}
	sendTransmission( t:Transmission ){
		console.log( JSON.stringify(t) );
		var toSend = this.encode(t).finish();
		this.#socket?.next( toSend );
	}
	send( m:any, log:string ):RequestId{
		const requestId = this.getRequestId();
		this.sendWithId( m, requestId, log );
		return requestId;
	}
	protected sendWithId( m:any, requestId:RequestId, log:string ):void{
		let t = new this.TCreator() as any;
		if( this.log.subRequest ) console.log( `[${requestId}]${log.substring(0, this.log.maxLength)}` );
		t["messages"].push( {requestId:requestId,...m} );
		const isAuthorization = Object.hasOwn( m, 'sessionId' );//the handshake message that releases the backlog; must go out before socketId is set
		if( this.#socket && (this.socketId || isAuthorization) )
			this.sendTransmission( t );
		else{
			this.backlog.push( t );
			if( !this.#socket )//open exactly one socket; sends before the ack queue rather than each opening a new connection
				this.connect();
		}
	}

	protected async sendAuthorization( socketId:number ):Promise<void>{
		let user = this.user()!;
		await this.sendPromise( {sessionId:user.authorization}, `sendAuthorization: ${user.authorization}` );
		this.setSocketId( socketId );//release buffer.
	}

	sendPromise<TResult>( m:any, log:string ):Promise<TResult>{
		const requestId = this.send( m, log );
		return new Promise<TResult>( ( resolve, reject )=>{
			this._callbacks.set( requestId, new RequestPromise(undefined, resolve, reject, null) );
		});
	}

	async initWait():Promise<void>{
		let p = new Promise<void>( (resolve,reject)=>this.#initCallbacks.push({resolve:resolve,reject:reject}) );
		await p;
	}

	async loginWait<Y>( target:string, log:Log=console.log ):Promise<Y>{
		let p = new Promise<Y>( (resolve,reject)=>{
			this.#loginCallbacks.push( {target: target, resolve:resolve, reject:reject, log:log} );
		});
		if( this.#loginCallbacks.length==1 ){
			let url = this.urlWithTarget( "serverSettings", true );
			if( this.log.restRequests ) log( url );
			let settings;
			try{
				let args = this.user()?.authorization ? {headers:{"Authorization":this.user()!.authorization}} : {} as any;
				const settings:any = await firstValueFrom( this.http.get<any>(url, args) );
				if( this.log.restResults ) log( JSON.stringify(settings) );
				this.timeoutSeconds = fromIsoDuration( settings["restSessionTimeout"] );
				let instance = parseInt( settings["serverInstance"] );
				let active = settings["active"];
				let timedout = this.lastRestCall && ( this.lastRestCall.getTime() < Date.now() - this.timeoutSeconds*1000 );
				let previousInstanceIndex = this.user()?.serverInstances?.findIndex( x=>x.url==this.url ) ?? -1;
				let previousInstance = previousInstanceIndex>=0 ? this.user()!.serverInstances![previousInstanceIndex].instance : 0;
				if( !active || timedout || (this.isAppServer && instance!=previousInstance) )
					this.authStore.reset( {url:this.url, instance:instance}, this.user()?.jwt );
				else if( previousInstance!=instance )
					this.authStore.setServerInstance( this.url, instance );
				for( let callback of this.#loginCallbacks ){
					let y = await this.authGet<any>(
						callback.target,
						this.user()!.authorization!,
						callback.log
					);
					callback.resolve( y );
				}
			}
			catch( e ){
				for( let callback of this.#loginCallbacks )
					callback.reject( e );
			}
			this.#loginCallbacks.length=0;
		}
		return p;
	}

	urlWithTarget( suffix:string, preferSecure:boolean=false ):string{
		return preferSecure && this.transport==ETransport.Hybrid
			? `${this.secureRestUrl}/${suffix}`
			: `${this.restUrl}/${suffix}`;
	}

	private async authGet<Y>( target:string, authorization?:string, log:Log=console.log ):Promise<Y>{
		if( target.indexOf("undefined")>=0 )
			debugger;
		if( this.log.restRequests )	log( decodeURIComponent(target).substring(0,this.log.maxLength) );
		let url = this.urlWithTarget(target);
		let y:Y;
		let options:any = {};
		if( authorization )
			options["headers"] = { "Authorization": authorization };
		if( !authorization || authorization.startsWith("Bearer ") ){
			options["observe"] = "response";
			options["transferCache"] = { includeHeaders: ["Authorization"] };
			try{
				let response = <HttpResponse<Y>>await firstValueFrom( this.http.get<Y>(url, options) );
				let newAuth = response.headers.get( "Authorization" );
				if( newAuth )
					this.authStore.append( {sessionId:newAuth} );
				y = response.body as Y;
			}
			catch( e:any ){
				if( e["status"]==401 && authorization ){//retry anonymously only if a (stale) credential was sent — an anonymous 401 must throw, or this recurses forever
					log( `(${e["status"]})${e["error"]} - ${e["url"]}` );
					this.authStore.logout();
					y = await this.authGet<Y>( target, undefined, log );
				}
				else
					throw e;
			}
		}
		else{
			try{
				y = await firstValueFrom( this.http.get<Y>(url, options) ) as Y;
			}
			catch( e:any ){
				if( e["status"]==401 && authorization ){//always true here (sessionId branch), but keeps the retry bound explicit
					log( `(${e["status"]})${e["error"]} - ${e["url"]}` );
					this.authStore.logout();
					y = await this.authGet<Y>( target, undefined, log );
				}
				else
					throw e;
			}
		}
		if( this.log.restResults ) log( JSON.stringify(y).substring(0,this.log.maxLength) );
		this.lastRestCall = new Date();
		return y;
	}

	async get<Y>( target:string, log?:Log ):Promise<Y>{
		if( !this.#instances )
			await this.initWait();
		let isActive = this.lastRestCall && (this.lastRestCall.getTime() > Date.now() - this.timeoutSeconds*1000);
		let y = !this.user()?.authorization || !isActive
			? await this.loginWait<Y>( target, log )
			: await this.authGet<Y>( target, this.user()!.authorization!, log );
		return y;
	}

	async loginJwt( credential:string ):Promise<string>{
		let options:any = {};
		options["headers"] = { "Authorization": `${credential}` };
		options["observe"] = "response";
		options["transferCache"] = { includeHeaders: ["Authorization"] };
		return await this.postRaw<string>( 'login', null, true, options );
	}

	async post<Y>( target:string, body:any, preferSecure:boolean=false ):Promise<Y>{
		return await this.postRaw<Y>( target, body, preferSecure );
	}

	async postRaw<Y>( target:string, body:any, preferSecure:boolean=false, options?:any ):Promise<Y>{
		if( !this.#instances )
			await this.initWait();
		const url = this.urlWithTarget( target, preferSecure );
		if( !options ){
			if( !this.user()?.authorization )
				options = {observe: "response", transferCache:{includeHeaders:["Authorization"]}};
			else{
				let authorization = this.user()?.authorization;
				if( authorization ){
					options = { headers:{"Authorization":authorization} };
					this.lastRestCall = new Date();
				}
			}
		}

		let event:HttpEvent<Y>|any = await firstValueFrom( this.http.post<Y>(url, body, options) );
		let y:Y;
		if( options.observe=="response" ){
			let response:HttpResponse<Y> = <HttpResponse<Y>>( event instanceof HttpResponse ? event : null );
			verify( response!=null, "response==null" );
			if( options?.transferCache?.includeHeaders.includes("Authorization") ){
				let authorization = response.headers.get( "Authorization" );
				verify( authorization!=null, "no authorization" );
				if( authorization )
					this.authStore.append( {sessionId:authorization} );
			}
			y = <Y>response?.body;
		}
		else
			y = <Y>event;
		return y;
	}

	async postQL<Y>( q:string, vars?:any, log:Log=console.log ):Promise<Y>{
		let args:any = {query: q};
		if( vars )
			args["variables"] = vars;
		if( this.log.restRequests ) log( `POST graphql/${JSON.stringify(args).substring(0,this.log.maxLength)}` );
		const y = await this.post<any>( `graphql`, args, false );
		if( this.log.restResults ) log( JSON.stringify(y).substring(0,this.log.maxLength) );
		return y ? y["data"] : null as unknown as Y;
	}
	async ql<Y>( q:Query, log:Log ):Promise<Y>{
		var target = `graphql?query={${q.text}}`;
		if( q.vars )
			target += `&variables=${encodeURIComponent( JSON.stringify(q.vars))}`;
		const y:any = await this.get( target, log );
		return y ? y["data"] as Y : null as unknown as Y;
	}

	async providers( log:Log ):Promise<EProvider[]>{
		const ql = `__type(name: "Provider") { enumValues { id name } }`;
		const data:any = await this.query( ql, null, log );
		return data["__type"]["enumValues"].map( (x:EnumValue)=>x.id );
	}
	async query<Y>( ql:string, vars?:any, log?:Log ):Promise<Y>{
		return await this.ql( {text: ql, vars:vars}, log ?? console.log );
	}
	async queryCount( ql:string, vars?:any, log?:Log ):Promise<number>{
		const y = await this.queryArray<{count:number}>( ql, vars, log ?? console.log );
		return y[0]["count"];
	}
	async querySingle<Y>( ql:string, vars?:any, log?:Log ):Promise<Y>{
		const y = await this.query<any>( ql, vars, log );
		return y[Object.keys(y)[0]];
	}
	async queryObject<Y>( ql:string, cnstrctr: new(...args:any[]) => Y, vars?:any, log?:Log ):Promise<Y>{
		const result = await this.query<any>( ql, vars, log );
		return new cnstrctr( result[Object.keys(result)[0]] );
	}
	async queryArray<Y>( ql:string, vars?:any, log?:Log ):Promise<Y[]>{
		const inputIndex = ql.indexOf('(');
		const fieldIndex = ql.indexOf('{');
		const index = inputIndex<0 ? fieldIndex : fieldIndex<0 ? inputIndex : Math.min( inputIndex, fieldIndex );
		const member = ql.substring( 0, index ).trim();
		const result:any = await this.ql( {text: ql, vars:vars}, log ?? console.log );
		if( !result.hasOwnProperty(member) )
			throw `'${member}' not found in ${JSON.stringify(result)}.`;
		const y = result[member];
		if( !Array.isArray(y) )
			throw `'${member}' is not an array in ${JSON.stringify(y)}.`;
		return y;
	}

	async querySetting( target:string, log:Log ):Promise<string>{
		const queryResult = await this.querySingle<{value:string}>( `setting(target:$target){value}`, {target: target}, log );
		return queryResult.value;
	}
	async querySettings(target:string[], log:Log):Promise<{[key:string]:string}>{
		const queryResult = await this.query<{settings:{target:string, value:string}[]}>( `settings(target:${JSON.stringify(target)}){target value}`, log );
		let y:{[key:string]:string} = {};
		for( const setting of queryResult.settings )
			y[setting.target] = setting.value;
		return y;
	}

	async mutate<Y>( ql: string|Mutation|Mutation[], log?:Log ):Promise<Y>{
		if( Array.isArray(ql) ){
			let y = [];
			for( let m of <Mutation[]>ql )
				y.push( await this.mutate(m, log) );
			return <Y>y;
		}
		let query = ql instanceof Mutation ? ql.toString() : ql;
		let vars = ql instanceof Mutation ? ql.variables : undefined;
		verify( query );
		return await this.postQL<Y>( `mutation ${query}`, vars, log );
	}

	async schemaWithEnums( type:string, log:Log ):Promise<TableSchema>{
		let schema = ( await this.schema([type], log) )[0];
		if( !schema.enums ){
			schema.enums = new Map<string, EnumValue[]>();
			for( const field of schema.fields.filter((x)=>x.type.underlyingKind==FieldKind.ENUM && !schema.enums.has(x.name)) ){
				let enumResult = await this.query<{__type: {enumValues: Array<EnumValue>;}}>(
					`__type(name: $type) { enumValues { id name } }`,
					{ type: field.type.name }
				);
				let values:Array<EnumValue>  = enumResult.__type["enumValues"];
				schema.enums.set( field.type.name, values );
			}
		}
		return schema;
	}
	async schema( types:string[], log?:Log ):Promise<TableSchema[]>{
		let results = new Array<TableSchema>();
		let queries =  new Array<string>();
		for( let type of types ){
			if( this.#tables.has(type) )
				results.push(this.#tables.get(type)!);
			else
				queries.push(type);
		};
		for( let type of queries ){
			const ql = `__type(name: $type) { fields { name type { name kind ofType{name kind} } } }`;
			const data:any = await this.query( ql, {type:type}, log );
			if( data["__type"].length==0 )
				throw `no such type: '${type}'`;
			const schema = new TableSchema( { ...data["__type"], name: type } );//keep the requested canonical type — the server may alias internal names (e.g. User → "UsersQl"), which broke collectionName/singular conventions downstream
			this.#tables.set( type, schema );
			results.push( schema );
		}
		return results;
	}

	async mutations():Promise<MutationSchema[]>{
		//NOTE: needs server-side `__schema{mutationType}` introspection, which the current backend rejects ("Query failed.") — callers must handle rejection.
		if( !this.#mutations ){
			const ql = `__schema{ mutationType{ name fields{ name args{ name defaultValue type{ name } } } } }`;//was missing a closing brace
			const data:any = await this.query( ql );
			this.#mutations = data.__schema.mutationType.fields;//was data.__schema.fields (always undefined)
		}
		return this.#mutations;
	}

	socketComplete(){ this.#socket = undefined; console.log( 'complete' ); }
	//get nextRequestId():RequestId{ return this.#requestId+1; }  why?
	getRequestId():RequestId{ return ++this.#requestId;} #requestId:RequestId=0;

	protected setSocketId( id:number ){
		this.#socketId = id;
		for( var m of this.backlog )
			this.sendTransmission( m );
		this.backlog.length=0;
	}
	private onMessage( event:MessageEvent ):protobuf.Buffer{
		const m = new Uint8Array( event.data );
		this.processMessage( m );
		return m;
	}

	processCommonMessage( m:any, requestId:RequestId ):boolean{
		let handled = true;
		let c = this._callbacks.get( requestId );
		if( c ){
			if( !m.Value ){//bare response (no oneof payload), e.g. the authorization ack
				this._callbacks.delete( requestId );//settled requests must be removed or the map grows for the socket's lifetime
				c.resolve( null );
			}
			else if( m["graphQl"] ){
				this._callbacks.delete( requestId );
				const json = m["graphQl"]["json"];//payload keyed by its field — Object.entries(m)[0][1] assumed decode order
				c.resolve( c.transformInput ? c.transformInput(json) : json );
			}
			else
				handled = false;//payload is for a subclass handler (subscriptionAck etc.) — leave the callback for it
		}
		else
			handled = false;
		return handled;
	}
/*	processCallback( id:number, resolution:any, log:string ){
		if( !this._callbacks.has(id) )
			throw `no callback for:  '${id}'`;
		if( this.log.restResults ) console.log( `(${id})${log}` );
		let p:RequestPromise<ResultMessage> = this._callbacks.get( id );
		p.resolve( resolution );
		this._callbacks.delete( id );
	}
*/
	processError( e:IException, requestId:RequestId ):boolean{
		const handled = this._callbacks.has( requestId );
		if( handled ){
			let p:RequestPromise<ResultMessage> = this._callbacks.get( requestId )!;
			p.reject( {error: {requestId:requestId, message:e.what as string, sc:e.code as number}} );
			this._callbacks.delete( requestId );
		}
		return handled;
	}
	protected abstract processMessage( bytearray:protobuf.Buffer ):void;

	protected abstract handleConnectionError( err:any ):void;
	protected abstract encode( t:Transmission ):any;

	protected backlog:Transmission[] = [];
	protected log = { sockRequests:true, sockResults:true, restRequests:true, restResults:true, subRequest:true, subResults:true, maxLength:255 };
	//Informational purposes only to match with server logs.
	protected get socketId():number{ return this.#socketId; } #socketId!:number;
	get instances(){return this.#instances;} set instances(x){
		this.#instances = x;
		for( let callback of this.#initCallbacks ){
			if( x.length )
				callback.resolve();
			else
				callback.reject( {error:{sc:0,message:"no server instances found."}} );
		}
	} #instances!:Instance[];
	#initCallbacks:{resolve:()=>void, reject:Reject}[]=[];
	#loginCallbacks:{target:string, resolve:(result:any)=>void, reject:( e:any )=>void, log:Log}[]=[];

	//abstract get queryId():number;
	#socket:WebSocketSubject<protobuf.Buffer>|undefined;
	protected _callbacks = new Map<number, RequestPromise<ResultMessage>>();
	#tables = new Map<string,TableSchema>();
	#mutations!:Array<MutationSchema>;//per-instance — a static cache shared one schema across AppService/AccessService/Gateway (different endpoints)
	private get url(){
		if( !this.instances?.length ) throw "no instances";
		return `${this.instances[0].host}:${this.instances[0].port}`;
	}
	protected get socketUrl(){ return `${this.transport==ETransport.Secure ? "wss" : "ws"}://${this.url}`; }
	private get restUrl(){return this.transport==ETransport.Secure ? this.secureRestUrl : `http://${this.url}`;}
	private get secureRestUrl(){return `https://${this.url}`;}

	isLoggedIn = computed( () => this.user()!= null );
	get user():Signal<User|undefined>{return this.authStore.user; }
	lastRestCall!:Date;
	timeoutSeconds!:number;
}