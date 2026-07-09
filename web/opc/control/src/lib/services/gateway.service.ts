import { Injectable, Inject, inject } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { HttpClient, HttpErrorResponse } from '@angular/common/http';
import { Subject,Observable, finalize } from 'rxjs';
import { AppService, AuthStore, Duration, IGraphQL, Guid, Instance, Log, MutationSchema, Mutation, ProtoService, ETransport, TableSchema, Timestamp, Type, Query } from 'jde-framework';
import { EProvider, User } from 'jde-spa';


import { OpcError } from '../model/OpcError';

import * as IotCommon from '../proto/Opc.Common'; import Common = IotCommon.Jde.Opc.Proto;
import * as IotRequests from '../proto/Opc.FromClient'; import FromClient = IotRequests.Jde.Opc.FromClient;
import * as IotResults from '../proto/Opc.FromServer'; import FromServer = IotResults.Jde.Opc.Gateway.FromServer;
import { OpcStore } from './opc-store';
import { NodeRoute } from '../model/NodeRoute';
import { CnnctnTarget, ServerCnnctn } from "../model/ServerCnnctn";
import { NodeKey, NodeId } from '../model/NodeId';
import { ENodeClass, ObjectType, OpcObject, UaNode, Variable } from '../model/Node';
import { OpcId, StatusCode } from '../model/types';
import { ExNodeId } from '../model/ExNodeId';
import { toValue, Value, valueJson } from '../model/Value';
import { Enum } from '../model/Enum';

interface IError{ requestId:number; message: string; }
type Owner = any;

export type GatewayTarget = string;
@Injectable( {providedIn: 'root'} )
export class GatewayService implements IGraphQL{
	constructor(
		@Inject("AuthStore") authStore:AuthStore,
		private route: ActivatedRoute,
		private router: Router,
		@Inject("OpcStore") opcStore:OpcStore ){
		this.appService.gatewayInstances().then(
			(instances)=>this.onGatewaySuccess( instances, this.appService.transport, this.http, authStore, opcStore ),
			(e)=>this.onInstancesError( e )//wrap so `this` is bound (bare method reference would run with this===undefined)
		);
		route.paramMap.subscribe( async params=>{
			const gatewayTarget = params.get('gateway');
			this.#defaultGateway = this.#gateways?.find( gateway=>gateway.target==gatewayTarget )!;
		});
		route.url.subscribe( async urlSegments=>{
			const url = urlSegments.map(segment=>segment.path).join("/");
			console.debug( `GatewayService:  url changed to '${url}'` );
		});
	}
	onGatewaySuccess(gateways:Instance[], transport:ETransport, http: HttpClient, authStore:AuthStore, opcStore:OpcStore){
		if( gateways.length==0 )
			console.error("No IotServies running");
		this.#gateways = gateways.map( instance=>new Gateway(instance, transport, http, authStore, opcStore) );
		this.#gatewaysCallbacks.forEach( cb=>cb.resolve(this.#gateways) );
		this.#gatewaysCallbacks = [];
		this.#gatewayCallbacks.forEach( cb=>cb.resolve(this.#gateways.find(gateway=>gateway.instances[0].instanceName==cb.instanceName)!) );
		this.#gatewayCallbacks = [];
	}
	onInstancesError(e:HttpErrorResponse){
		console.error( `Could not get IotServices.  (${e.status})${e.message}` );
		this.#gatewaysCallbacks.forEach( x=>x.reject(e) );//reject BOTH lists so gateways() awaiters don't hang, not just gateway()
		this.#gatewaysCallbacks = [];
		this.#gatewayCallbacks.forEach( x=>x.reject(e) );
		this.#gatewayCallbacks = [];
	}
	async gateway( instanceName:string ):Promise<Gateway>{
		if( !this.#gateways )
			return new Promise<Gateway>( (resolve,reject)=>this.#gatewayCallbacks.push({instanceName, resolve, reject}) );
		return this.#gateways.find( gateway=>gateway.instances[0].instanceName==instanceName )!;
	}
	async gateways():Promise<Gateway[]>{
		if( !this.#gateways )
			return new Promise<Gateway[]>( (resolve,reject)=>this.#gatewaysCallbacks.push({resolve:resolve,reject:reject}) );
		return Promise.resolve( this.#gateways );
	}
	async ql<Y>( q:Query, log:Log ):Promise<Y>{ return this.defaultGateway.ql( q, log ); }
	async query<T>( ql: string ):Promise<T>{ return this.defaultGateway.query<T>( ql ); }
	async querySingle<T>( ql: string ):Promise<T>{ return this.defaultGateway.querySingle<T>( ql ); }
	async schema( names:string[] ):Promise<TableSchema[]>{ return this.defaultGateway.schema( names ); }
	async schemaWithEnums( type:string, log:Log ):Promise<TableSchema>{ return this.defaultGateway.schemaWithEnums( type, log ); }
	async mutate<T>( ql: string|Mutation|Mutation[], log?:Log ):Promise<T>{
		return this.defaultGateway.mutate<T>( ql, log );
	}
	async mutations():Promise<MutationSchema[]>{ return this.defaultGateway.mutations(); }

	targetQuery( schema:TableSchema, target: string, showDeleted:boolean ):string{ return null as any; }
	subQueries( typeName: string, id: number ):string[]{ return []; }
	excludedColumns( tableName:string ):string[]{ return []; }
	toCollectionName( collectionDisplay:string ):string{ return collectionDisplay; }

	get defaultGateway():Gateway{
		if( !this.#defaultGateway ){
			let target = this.router.url.split('/').slice(-1)[0];
			this.#defaultGateway = this.#gateways?.find( gateway=>gateway.target==target )!;
		}
		return this.#defaultGateway;
	} #defaultGateway!:Gateway;
	#gateways!:Gateway[];

	#gatewaysCallbacks:{resolve: (value:Gateway[])=>void, reject:(e?:any)=>void}[]= [];
	#gatewayCallbacks:{ instanceName:string, resolve: (value:Gateway)=>void, reject:(e?:any)=>void}[]= [];
	appService = inject(AppService);
	http = inject(HttpClient);
}


export class Gateway extends ProtoService<FromClient.ITransmission,FromServer.IMessage>{
	constructor( gateway:Instance, transport:ETransport, http: HttpClient, authStore:AuthStore, private store:OpcStore ){
		super( FromClient.Transmission, http, transport, authStore );
		super.instances = [gateway];
		super.queryArray<ServerCnnctn>( `serverConnections{id target name url certificateUri defaultBrowseNs}`, null, (x)=>console.log(x) ).then( connections=>{
			connections.forEach( c=>this.#connections.set(c.target, new ServerCnnctn(c as any)) );
		});
	}
	async login( domain:string, username:string, password:string, log:Log ):Promise<void>{
		let self = this;
		if( this.log.restRequests )	console.log( `Login( opc='${domain}', username='${username}' )` );
		try{
			await this.logout( log );
			console.assert( !this.user()?.authorization );
			await this.postRaw<any>( 'login', {opc:domain, user:username, password:password}, true, null );
			if( this.log.restResults ) console.log( `authorization: '${this.user()?.authorization}'` );
			let user = new User();
			user.id = domain ? `${domain}\\${username}` : username;
			user.domain = domain;
			user.name = username;
			user.provider = EProvider.OpcServer;
			this.authStore.append( user );
		}
		catch( e ){
			throw e;
		}
	}

	async logout( log:Log ):Promise<void>{
		let self = this;
		if( this.log.restRequests )	console.log( `logout()` );
		try{
			await this.postRaw<string>( 'logout', {}, false, {} );
			if( this.log.restResults ) console.log( `logout` );
		}
		catch( e:any ){
			log( `logout failed:  ${e["message"] ?? "Unknown error"}` );
		}
		this.authStore.logout();
	}

	protected encode( t:FromClient.Transmission ){ return FromClient.Transmission.encode(t); }
	protected handleConnectionError(){};
	protected processMessage( buffer:protobuf.Buffer ){
		try{
			const transmission = FromServer.Transmission.decode( buffer );
			for( const message of <FromServer.Message[]>transmission.messages ){
				let requestId = message.requestId;
				if( super.processCommonMessage(message, requestId) )
					continue;
				if( message.ack ){
					console.log( `[App.${requestId}]Connected to '${super.socketUrl}', socketId: ${message.ack}` );
					let socketId = message.ack;
					if( this.user()?.authorization )
						super.sendAuthorization( socketId );
					else{
						console.warn( `no authorization` );
						this.setSocketId( socketId );
					}
				}
				else if( message.nodeValues )
					this.nodeValues( message.nodeValues );
				else if( message.subscriptionAck )
					this.subscriptionAck( requestId, message.subscriptionAck );
				else if( message.unsubscribeAck )
					this.onUnsubscriptionResult( requestId, message.unsubscribeAck );
				else if( message.exception ){
					const e = message.exception;
					if( !this.processError( e, requestId ) )
						throw e;
				}
				else
					throw `unknown message:  ${JSON.stringify( message[message.Value!] )}`;
			}
		}
		catch( e ){
			if( e instanceof String )
				console.error( e );
			else
				console.error( e );
		}
	}
	private static toParams( obj:any ){
		let params="";
		Object.keys(obj).forEach( m=>{if(params.length)params+="&"; params+=`${m}=${obj[m]}`;} );
		return params;
	}
	private static toNode( proto:Common.INodeId ):NodeId{
		let node = new NodeId( {ns:proto.namespaceIndex} );
		switch( proto.Identifier ){//oneof discriminator: identifies the set field even when its value is 0/"" (falsy)
			case "numeric":    node.id = proto.numeric!; break;
			case "string":     node.id = proto.string!; break;
			case "byteString": node.id = proto.byteString!; break;
			case "guid":       node.id = Gateway.toGuid( proto.guid! ); break;
		}
		return node;
	}
	private static toExpanded( proto:Common.IExpandedNodeId ):ExNodeId{
		const en = new ExNodeId( {nsu:proto.namespaceUri!, serverIndex:proto.serverIndex!} );
		const n = Gateway.toNode(proto.node!);
		en.id = n.id;
		en.ns = n.ns;
		return en;
	}

	private static toProto( nodes:NodeId[] ):Common.INodeId[]{
		let protoNodes = [];
		for( const node of nodes ){
			//Subscribe/Unsubscribe.nodes are plain Proto.NodeId — the previous ExpandedNodeId{node:…} wrapper encoded as an EMPTY node (fields read off the wrapper), so every subscribe failed with BadNodeIdUnknown
			let proto = new Common.NodeId();
			proto.namespaceIndex = node.ns;
			if( typeof node.id === "number" )
				proto.numeric = node.id;
			else if( typeof node.id === "string" )
				proto.string = node.id;
			else if( node.id instanceof Guid )
				proto.guid = node.id.value;
			else if( node.id instanceof Uint8Array )
				proto.byteString = node.id;
			protoNodes.push( proto );
		}
		return protoNodes;
	}

	private async updateErrorCodes(){
		const scs = OpcError.emptyMessages();
		if( scs.length ){
			const json = await super.get( `ErrorCodes?scs=${scs.join(',')}` ) as any;
			OpcError.setMessages( json["errorCodes"] );
		}
	}
	async errorCodeText( sc:StatusCode ):Promise<string>{
		let text = OpcError.statusCodeText( sc );
		if( !text ){
			await this.updateErrorCodes();
			text = OpcError.statusCodeText( sc );
		}
		return `(${sc.toString(16)})${text}`;
	}

	public async browseObjectsFolder( cnnctn:CnnctnTarget, parent:UaNode, snapshot:boolean, log:Log ):Promise<UaNode[]>{
		if( parent.isVariable )
			throw new EvalError( `Cannot browse children of variable node.`, {cause:"Invalid Operation"} );
		const vars = { opc: cnnctn, id: parent.nodeId.toJson() };
		const commonColumns = "id name browse nodeClass refType typeDef description";
		const variableColumns = "dataType value valueRank accessLevel userAccessLevel";
		const ql = `node(opc:$opc, id:$id){children{${commonColumns} ... on Variable{${variableColumns}} }}`;
		const children = (await this.query<any>( ql, vars, (m)=>console.log(m) ))["node"]["children"];
		var y = new Array<UaNode>();
		for( const ref of children ){
			let child:UaNode|undefined;
			switch( <ENodeClass>ref.nodeClass ){
				case ENodeClass.Object: child = new OpcObject(ref, parent); break;
				case ENodeClass.ObjectType: parent.typeDef = new ObjectType(ref); break; //y.push( new ObjectType(ref) ); break;
				case ENodeClass.Variable:
					let variable = new Variable(ref, parent);
					child = variable;
					if( variable.customDataType )
						try{
							const nodeId = <NodeId>variable.customDataType
							let x = await super.querySingle<Type>( `__type( opc: "${cnnctn}", ${nodeId.qlArgs()}){ enumValues{id name description}}`, null, log );
							variable.customDataType = new Enum(nodeId, x);
						}
						catch( e:any ){
							log( e["message"] );
						}
					break;
				default: console.error( `browseObjectsFolder - unhandled nodeClass ${ref.nodeClass} for '${ref.name}'` );//was `debugger;` — froze the app whenever a debugger (DevTools/automation) was attached
			}
			if( child )
				y.push( child );
		}
		this.store.setNodes( this.target, cnnctn, parent, y );
		this.updateErrorCodes();
		return y;
	}
	async snapshot( opcId:CnnctnTarget, nodes:NodeId[] ):Promise<Map<NodeId,Value>>{
		const results = await super.queryArray<{id:NodeId,value:Value}>( `nodes( opc: "${opcId}", id:[${NodeId.qlArgsArray(nodes)}]){id value}` );
		var y = new Map<NodeId,Value>();
		for( const snapshot of results )
			y.set( new NodeId(snapshot.id), toValue(snapshot.value) );
		this.updateErrorCodes();
		return y;
	}
	async read( opcId:CnnctnTarget, n:NodeId ):Promise<Value>{
		const v = super.querySingle<Value>( `node( opc: "${opcId}", ${n.qlArgs()}){value}` );
		return v;
	}
	async write( opcId:CnnctnTarget, n:NodeId, v:Value, log:Log ):Promise<Value>{
		const q = `updateVariable( opc: $opc, id: $id, value: $value ){ value }`;
		const vars = { opc: opcId, id: n.toJson(), value: valueJson(v) };
		const newValue:any = toValue( await super.postQL<Value>(q, vars, log) );
		this.updateErrorCodes();
		return newValue["value"];
	}

	setRoute(route: NodeRoute){
		this.store.setRoute( route, this.#connections.get(route.cnnctnTarget)?.defaultBrowseNs );
	}

	private onUnsubscriptionResult( requestId:number, result:FromServer.IUnsubscribeAck ){
		result.failures?.forEach( (node)=>console.log(`unsubscribe failed for:  ${JSON.stringify(node)}`) );
		const c = this._callbacks.get( requestId );
		if( c ){
			this._callbacks.delete( requestId );//settled requests must be removed or the map grows for the socket's lifetime
			c.resolve( null );
		}
	}

	private subscriptionAck( requestId:number, ack:FromServer.ISubscriptionAck ){
		const c = this._callbacks.get( requestId );
		if( c ){
			this._callbacks.delete( requestId );
			c.resolve( ack.results );
		}
	}

	private async _subscribe( opcId:OpcId, nodes:NodeId[], subject:Subject<SubscriptionResult> ):Promise<void>{
		const request:FromClient.ISubscribe = { nodes:Gateway.toProto(nodes), opcId:opcId };
		let toDelete = new Array<NodeId>();
		try{
		 	let y = await this.sendPromise<FromServer.IMonitoredItemCreateResult[]>( {"subscribe":request}, `subscribe opcId: ${opcId}, nodeCount: ${nodes.length}` );
		 	for( let i=0; i<y.length; ++i ){
				const node = nodes[i];
				if( y[i].statusCode ){
					toDelete.push( node );
					let e = new OpcError( y[i].statusCode!, "Subscribe", new Error().stack!, undefined );
					console.log( `Subscription failed for '${node} - ${e}` );
				}
		 	}
		}
		catch( e:any ){
			console.error( e["error"]["message"] );
			toDelete.push( ...nodes );
			subject.error( e );
		}
		toDelete.forEach( (n)=>this.getOpcSubscriptions(opcId).delete(n.key) );
	}

	#ownerSubscriptions = new Map<Owner,Subject<SubscriptionResult>>();
	#subscriptions = new Map<OpcId,Map<NodeKey, Owner[]>>();
	getOpcSubscriptions(opcId:OpcId):Map<NodeKey, Owner[]>{ return this.#subscriptions.has( opcId ) ? this.#subscriptions.get( opcId )! : this.#subscriptions.set( opcId, new Map<NodeKey, Owner[]> ).get( opcId )!; }
	#nodes = new Map<NodeKey, NodeId>();
	private clearUnusedNodes(){
		this.#nodes.forEach( (_, key)=>{
			const keys = [...this.#subscriptions.entries()].filter( ({1:value})=>value.has(key) ).map( ([key])=>key );
			if( !keys.length )
				this.#nodes.delete(key);
		});
	}
	public addToSubscription( opcId:OpcId, nodes:NodeId[], owner:Owner ){
		let opcSubscriptions = this.getOpcSubscriptions( opcId );
		for( const node of nodes ){
			let owners:Owner[];
			if( opcSubscriptions.has(node.key) ){
				let owners = opcSubscriptions.get( node.key )!;
				if( !owners.includes(owner) )
					owners.push( owner );
			}
			else{
				opcSubscriptions.set( node.key, [owner] ).get( node.key );
				this.#nodes.set( node.key, node );
			}
		}
		let subject = this.#ownerSubscriptions.has( owner ) ? this.#ownerSubscriptions.get( owner )! : this.#ownerSubscriptions.set( owner, new Subject<SubscriptionResult>() ).get( owner )!;
		this._subscribe( opcId, nodes, subject );
	}
	subscribe( opcId:OpcId, nodes:NodeId[], owner:Owner ):Observable<SubscriptionResult>{
		this.addToSubscription( opcId, nodes, owner );
		let subject = this.#ownerSubscriptions.get( owner )!;
		return subject.pipe(
			finalize(() => {//https://stackoverflow.com/questions/62579473/detect-when-a-subject-has-no-more-subscriptions
				if( !subject.observers.length ) {
					this.clearOwner( owner );
				}
			}));
	}

	private static toGuid( proto:Uint8Array ):Guid{ let guid = new Guid(); guid.value = proto; return guid; }
	private static toValue( proto:FromServer.IValue ):Value{
		switch( proto.of ){//oneof discriminator: dispatch on the set field, not its truthiness, so false/0/"" are not dropped
			case "boolean":      return proto.boolean!;
			case "byte":         return proto.byte!;
			case "byteString":   return proto.byteString!;
			case "date":         return <Timestamp>proto.date;
			case "doubleValue":  return proto.doubleValue!;
			case "duration":     return <Duration>proto.duration;
			case "expandedNode": return Gateway.toExpanded( proto.expandedNode! );
			case "floatValue":   return proto.floatValue!;
			case "guid":         return Gateway.toGuid( proto.guid! );
			case "int16":        return proto.int16!;
			case "int32":        return proto.int32!;
			case "int64":        return proto.int64!;
			case "node":         return Gateway.toNode( proto.node! );
			case "sbyte":        return proto.sbyte!;
			case "statusCode":   return proto.statusCode!;
			case "stringValue":  return proto.stringValue!;
			case "uint16":       return proto.uint16!;
			case "uint32":       return proto.uint32!;
			case "uint64":       return proto.uint64!;
			case "xmlElement":   return proto.xmlElement!;
		}
		return undefined!;
	}
	private static toValues( proto:FromServer.IValue[] ):Value{
		let value = proto.length==1 ? Gateway.toValue( proto[0] ) : new Array<Value>();
		if( proto.length>1 )
			proto.forEach( v => (<Value[]>value).push( Gateway.toValue(v) ) );
		return value;
	}

	private nodeValues( nodeValues:FromServer.INodeValues ):void{
		let opcSubscriptions = this.#subscriptions.get( nodeValues.opcId! ); if( !opcSubscriptions ){ return console.error(`Could not find opc ${nodeValues.opcId}`);}
		const node = Gateway.toNode( nodeValues.node! );
		opcSubscriptions.get( node.key )?.forEach( owner=>this.#ownerSubscriptions.get(owner)!.next({opcId:nodeValues.opcId!, node:node, value:Gateway.toValues(nodeValues.values!)}) );
	};

	private clearOwnerNode( opcSubscriptions:Map<NodeKey, Owner[]>,  key:NodeKey, owner:Owner ){
		let owners = opcSubscriptions.get( key )!;
		const index = owners.indexOf( owner );
		let tombStone = false;
		if( index!=-1 ){
			owners.splice( index, 1 );
			tombStone = !owners.length
			if( tombStone )
				opcSubscriptions.delete( key );
		}
		return tombStone;
	}
	// remove all subscriptions for owner.
	private clearOwner( owner:Owner ){
		this.#ownerSubscriptions.delete( owner );
		let toDeleteKeys = new Map<OpcId,NodeKey[]>();
		for( const [opcId, opcSubscriptions] of this.#subscriptions.entries() ){
			for( const nodeKey of opcSubscriptions.keys() ){
				if( this.clearOwnerNode(opcSubscriptions, nodeKey, owner) )
				toDeleteKeys.has(opcId) ? toDeleteKeys.get(opcId)!.push(nodeKey) : toDeleteKeys.set( opcId, [nodeKey] );
			}
		}
		let toDeleteNodes = new Map<OpcId,NodeId[]>();
		for( const [opcId,keys] of toDeleteKeys ){
			let nodes = toDeleteNodes.set( opcId, [] ).get( opcId )!;
			keys.forEach( key=>nodes.push( this.#nodes.get(key)! ) );
		}
		if( toDeleteNodes.size ){
			for( const [opcId, nodes] of toDeleteNodes ){
				var request:FromClient.IUnsubscribe = { nodes:Gateway.toProto(nodes), opcId:opcId };
				this.sendPromise<void>( {"unsubscribe": request}, `unsubscribe opcId: ${opcId}, nodeCount: ${nodes.length}` );
			}
		}
		this.clearUnusedNodes();
	}
	// Unsuscribe, but keep subscription open.
	async unsubscribe( opcId:string, nodes:NodeId[], owner:Owner ):Promise<void>{
		let opcSubscriptions = this.#subscriptions.get( opcId )!;
		let toDelete = new Array<NodeId>();
		for( const node of nodes ){
			if( this.clearOwnerNode( opcSubscriptions, node.key, owner ) )
				toDelete.push( node );
		}

		this.clearUnusedNodes();
		if( toDelete.length ){
			var request:FromClient.IUnsubscribe = { nodes:Gateway.toProto(toDelete), opcId:opcId };
			return this.sendPromise<void>( {"unsubscribe": request}, `unsubscribe opcId: ${opcId}, nodeCount: ${toDelete.length}` );
		}
		else
			return Promise.resolve();
	}
	get name():string{ return this.instances[0].instanceName!; }
	get target():GatewayTarget{ return this.instances[0].instanceName!; }
	#connections = new Map<CnnctnTarget, ServerCnnctn>();
}
export type SubscriptionResult = {opcId:string, node:NodeId,value:Value};