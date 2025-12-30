import { Injectable } from "@angular/core";
import { CnnctnTarget, ServerCnnctn } from "../model/ServerCnnctn";
import { browseEq, ETypes, Ns, toBrowse } from '../model/types';
import { NodeRoute } from "../model/NodeRoute";
import { OpcObject, UaNode, ENodeClass } from "../model/Node";
import { NodeId, NodeKey } from "../model/NodeId";
import { DocItem } from "jde-spa";
import { Gateway, GatewayTarget } from "./gateway.service";
import { Server, ServerProps } from "../model/Server";

class StoreNode{
	constructor( node:UaNode ){
		this.node = node;
	}
	parentId:NodeId;
	node:UaNode;
	children:UaNode[] = [];
}

@Injectable({providedIn: 'root'})
export class OpcStore{
	constructor(){
		console.log( `OpcStore::OpcStore` );
	}

	public async getConnection( gatewayService:Gateway, cnnctn:CnnctnTarget ):Promise<Server>{
		//#connections = new Map<GatewayTarget,Map<CnnctnTarget, ServerCnnctn>>();
		const gateway = gatewayService.target;
		let gatewayConnections = this.#connections.get( gateway );
		if( !gatewayConnections )
			this.#connections.set( gateway, gatewayConnections = new Map<CnnctnTarget, Server>() );
		if( gatewayConnections.has(cnnctn) )
			return gatewayConnections.get(cnnctn);
		let q = `\
			connection: serverConnection( target: $opc){ id name target url certificateUri defaultBrowseNs }
			desc: serverDescription( opc: $opc ){ applicationUri productUri applicationName applicationType gatewayServerUri discoveryProfileUri discoveryUrls }
			policy: securityPolicyUri( opc: $opc )
			mode: securityMode( opc: $opc )`;
		let props = await gatewayService.query<ServerProps>(q, {opc:cnnctn}, (m)=>console.log(m));
		let server = new Server( props );
		gatewayConnections.set( cnnctn, server );
		return server;
	}

	private getNodes( gateway:GatewayTarget, cnnctn:CnnctnTarget ):Map<NodeKey,StoreNode>{
		let gatewayNodes = this.#nodes.get( gateway );
		if( !gatewayNodes )
			this.#nodes.set( gateway, gatewayNodes = new Map<CnnctnTarget, Map<NodeKey,StoreNode>>() );
		let nodes = gatewayNodes.get( cnnctn );
		if( !nodes ){
			gatewayNodes.set( cnnctn, nodes = new Map<NodeKey,StoreNode>() );
			nodes.set( OpcObject.rootNode.key, new StoreNode(new OpcObject({ns: OpcObject.rootNode.nodeId.ns, id: OpcObject.rootNode.nodeId.id, browse: '', name: cnnctn, nodeClass: ENodeClass.Object, typeDefinition: ETypes.Folder})) );
		}
		return nodes;
	}
	private findStore( gateway:GatewayTarget, cnnctn:CnnctnTarget, node:NodeId ):StoreNode{
		const opcNodes = this.#nodes.get( gateway )?.get( cnnctn );
		let store:StoreNode;
		if( opcNodes )
			store = opcNodes.get( node.key );
		return store;
	}
	private getStore( nodes:Map<NodeKey,StoreNode>, node:UaNode ):StoreNode{
		let store = nodes.get( node.key );
		if( !store ){
			// if( OpcObject.rootNode.equals(node) )
			// 	throw new EvalError( `Root node not set.`, {cause:"Internal Error"} );
			// this.addChildren( nodes, node.parent, [node] );
			nodes.set( node.key, store = new StoreNode(node) );
		}
		return store;
	}
	setServerCnnctns( clients: DocItem[] ):void{
		this.#serverCnnctnRoutes = [...clients];
		for( let route of this.#serverCnnctnRoutes )
			route.path = route.path.substring( route.path.lastIndexOf("/")+1 );
	}
	getParent( opcNodes:Map<NodeKey,StoreNode>, path:string, defaultNs:Ns ):UaNode{
		let segments = path.split("/");
		if( segments.length==1 )
			return OpcObject.rootNode;

		let parent = opcNodes.get( OpcObject.rootNode.key );
		for( let segment of segments.slice(0, -1) ){
			if( !parent )
				return null;
			let child = parent.children.find( (c)=>browseEq(c.browse, toBrowse(segment, defaultNs)) );
			if( !child )
				return null;
			parent = opcNodes.get( child.key );
		}
		if( !parent )
			throw new EvalError( `Parent not set for '${path}'`, {cause:"Internal Error"} );
		return parent?.node;
	}
	insertNode( route:NodeRoute, parents:any, defaultNs:Ns ):void{
		let opcNodes = this.getNodes( route.gatewayTarget, route.cnnctnTarget );
		for( let parent of parents ){
			parent.browse = toBrowse( parent.path, defaultNs );
			let obj = new OpcObject( parent );
			if( !obj.parent )
				obj.parent = this.getParent( opcNodes, parent.path, defaultNs );
			if( !opcNodes.has(obj.key) )
				this.addChildren( opcNodes, obj.parent, [obj] );
		}

		if( !route.node.parent )
			route.node.parent = this.getParent( opcNodes, route.path, defaultNs );
		if( !opcNodes.has(route.node.key) )
			this.addChildren( opcNodes, route.node.parent, [route.node] );
	}

	setNodes( gateway:GatewayTarget, cnnctn:CnnctnTarget, parent:UaNode, children:UaNode[] ){
		let opcNodes = this.getNodes( gateway, cnnctn );
		this.addChildren( opcNodes, parent, children );
	}
	private addChildren( opcNodes:Map<NodeKey,StoreNode>, parent:UaNode, children:UaNode[] ){
		let store = this.getStore( opcNodes, parent );
		store.children = [];
		for( let child of children ){
			store.children.push( child );
			let childStore = this.getStore( opcNodes, child );
			childStore.parentId = parent.nodeId;
			childStore.node = child;
		}
	}
	setRoute(route: NodeRoute, defaultBrowseNs:Ns|null ):void{
		if( route.node.equals(OpcObject.rootNode) ){
			route.siblings = [new DocItem({title: route.cnnctnTarget, path: route.cnnctnTarget})]; //TODO add all connections.
			return;
		}
		let findStore = (node:NodeId):StoreNode => {
			return node ? this.findStore( route.gatewayTarget, route.cnnctnTarget, node ) : null;
		};
		const store = findStore( route.nodeId );
		let parentPaths = [];
		let parent = findStore( store?.parentId );
		for( let current = parent; current && !current.node.equals(OpcObject.rootNode); current=findStore(current.parentId) ){
			parentPaths.push( current.node.browseFQ(defaultBrowseNs) );
		}
		if( !parent )
			throw new EvalError( `Parent not found for ${store?.node.browse}`, {cause:"Internal Error"} );

		route.parent = new DocItem( {path: `${route.cnnctnTarget}/${parentPaths.reverse().join('/')}`, title: parent.node.name ?? route.cnnctnTarget} );
		route.siblings = [];
		for( const sibling of parent.children ){
			const siblingStore = sibling.key == route.nodeId.key ? store : findStore( sibling.nodeId );
			const siblingRef = siblingStore?.node;
			if( siblingRef?.isObject && siblingRef?.displayed )
				route.siblings.push( new DocItem({path: `${route.parent.path}/${siblingRef.browseFQ(defaultBrowseNs)}`, title: siblingRef.name}) );
		}
	}
	findNodeId( gateway:string, cnnctnTarget:string, browsePath:string ): UaNode {
		let nodes = this.getNodes(gateway, cnnctnTarget);
		let storeNode: StoreNode = nodes.get( OpcObject.rootNode.key );
		if( !storeNode )
			return null;
		let uaNode: UaNode;
		let cnnctn = this.#connections.get( gateway )?.get( cnnctnTarget );
		browsePath.split("/").forEach( (segment, i)=>{
			uaNode = storeNode.children.find( (c)=>browseEq(c.browse, toBrowse(segment, cnnctn?.connection.defaultBrowseNs)) );
			if( uaNode )
				storeNode = this.findStore( gateway, cnnctnTarget, uaNode.nodeId );
		} );
		return uaNode;
	}

	#serverCnnctnRoutes: DocItem[];
	#nodes = new Map<GatewayTarget,Map<CnnctnTarget, Map<NodeKey,StoreNode>>>();
	#connections = new Map<GatewayTarget,Map<CnnctnTarget, Server>>();
}