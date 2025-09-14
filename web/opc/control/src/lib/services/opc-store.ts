import { Injectable } from "@angular/core";
import { CnnctnTarget, ServerCnnctn } from "../model/ServerCnnctn";
import { ETypes } from '../model/types';
import { NodeRoute } from "../model/NodeRoute";
import { OpcObject, UaNode, ENodeClass } from "../model/Node";
import { NodeId, NodeKey } from "../model/NodeId";
import { DocItem } from "jde-spa";
import { Gateway, GatewayTarget } from "./gateway.service";

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

	public async getConnection( gatewayService:Gateway, cnnctn:CnnctnTarget ):Promise<ServerCnnctn>{
		//#connections = new Map<GatewayTarget,Map<CnnctnTarget, ServerCnnctn>>();
		const gateway = gatewayService.target;
		let gatewayConnections = this.#connections.get( gateway );
		if( !gatewayConnections )
			this.#connections.set( gateway, gatewayConnections = new Map<CnnctnTarget, ServerCnnctn>() );
		if( gatewayConnections.has(cnnctn) )
			return gatewayConnections.get(cnnctn);
		let connection = new ServerCnnctn( (await gatewayService.query<any>(`serverConnection( target: "${cnnctn}"){ id name url certificateUri defaultBrowseNs }`, (m)=>console.log(m)))["serverConnection"] );
		gatewayConnections.set( cnnctn, connection );
		return connection;
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

	insertNode( gateway:GatewayTarget, cnnctn:CnnctnTarget, node:UaNode ):void{
		let opcNodes = this.getNodes( gateway, cnnctn );
		if( !node.parent && !node.equals(OpcObject.rootNode) )
			throw new EvalError( `Parent not set for ${node.browse}`, {cause:"Internal Error"} );
		if( !opcNodes.has(node.key) )
			this.addChildren( opcNodes, node.parent, [node] );
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
	setRoute(route: NodeRoute) {
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
			parentPaths.push( current.node.browse );
		}
		if( !parent )
			throw new EvalError( `Parent not found for ${store?.node.browse}`, {cause:"Internal Error"} );

		route.parent = new DocItem( {path: `${route.cnnctnTarget}/${parentPaths.reverse().join('/')}`, title: parent.node.name ?? route.cnnctnTarget} );
		route.siblings = [];
		for( const sibling of parent.children ){
			const siblingStore = sibling.key == route.nodeId.key ? store : findStore( sibling.nodeId );
			const siblingRef = siblingStore?.node;
			if( siblingRef?.isObject && siblingRef?.displayed )
				route.siblings.push( new DocItem( {path: `${route.cnnctnTarget}/${siblingRef.browse}`, title: siblingRef.name} ) );
		}
	}
	findNodeId( gateway:string, cnnctnTarget:string, browsePath:string ): UaNode {
		let uaNode: UaNode;
		let nodes = this.getNodes(gateway, cnnctnTarget);
		let storeNode: StoreNode = nodes.get( OpcObject.rootNode.key );
		if( !storeNode )
			return uaNode;
		browsePath.split("/").forEach( (segment, i)=>{
			uaNode = storeNode.children.find( (c)=>c.browse == segment );
			if( uaNode )
				storeNode = this.findStore( gateway, cnnctnTarget, uaNode.nodeId );
		} );
		return uaNode;
	}

	#serverCnnctnRoutes: DocItem[];
	#nodes = new Map<GatewayTarget,Map<CnnctnTarget, Map<NodeKey,StoreNode>>>();
	#connections = new Map<GatewayTarget,Map<CnnctnTarget, ServerCnnctn>>();
}