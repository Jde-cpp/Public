import { Injectable } from "@angular/core";
import { CnnctnTarget } from "../model/ServerCnnctn";
import { ETypes } from '../model/types';
import { NodeRoute } from "../model/NodeRoute";
import { OpcObject, UaNode } from "../model/Node";
import { NodeId, NodeKey } from "../model/NodeId";
import { DocItem } from "jde-spa";
import { GatewayTarget } from "./gateway.service";

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
	private getNodes( gateway:GatewayTarget, cnnctn:CnnctnTarget ):Map<NodeKey,StoreNode>{
		let gatewayNodes = this.#nodes.get( gateway );
		if( !gatewayNodes )
			this.#nodes.set( gateway, gatewayNodes = new Map<CnnctnTarget, Map<NodeKey,StoreNode>>() );
		let nodes = gatewayNodes.get( cnnctn );
		if( !nodes )
			gatewayNodes.set( cnnctn, nodes = new Map<NodeKey,StoreNode>() );
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
		let store = nodes.get( node.nodeId.key );
		if( !store )
			nodes.set( node.nodeId.key, store = new StoreNode(node) );
		return store;
	}
	setServerCnnctns( clients: DocItem[] ){
		this.#serverCnnctnRoutes = [...clients];
		for( let route of this.#serverCnnctnRoutes )
			route.path = route.path.substring( route.path.lastIndexOf("/")+1 );
	}

	setNodes( gateway:GatewayTarget, cnnctn:CnnctnTarget, node:UaNode, children:UaNode[] ){
		let opcNodes = this.getNodes( gateway, cnnctn );
		let store = this.getStore( opcNodes, node );
		store.children = [];
		for( let child of children ){
			store.children.push( child );
			let childStore = this.getStore( opcNodes, child );
			childStore.parentId = node.nodeId;
			childStore.node = child;
		}
	}
	setRoute(route: NodeRoute) {
		if( route.node.equals(OpcObject.rootNode) ){
			route.siblings = [new DocItem({title: route.cnnctnTarget, path: route.cnnctnTarget})]; //TODO add all connections.
			return;
		}
		let findStore = (node:NodeId):StoreNode => { return node ? this.findStore( route.gatewayTarget, route.cnnctnTarget, node ) : null; };
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
			const siblingStore = sibling.nodeId.key == route.nodeId.key ? store : findStore( sibling.nodeId );
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
}