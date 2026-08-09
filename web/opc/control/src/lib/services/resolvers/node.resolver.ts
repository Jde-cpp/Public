import { Inject, Injectable } from '@angular/core';
import { ActivatedRouteSnapshot, createUrlTreeFromSnapshot, Params, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { SnackbarService } from 'jde-framework';
import { Gateway, GatewayService } from '../gateway.service';
import { OpcObject, UaNode } from '../../model/Node';
import { NodeRoute } from '../../model/NodeRoute';
import { OpcStore } from '../opc-store';
import { Server } from '../../model/Server';

export type NodePageData = {
	route:NodeRoute;
	nodes:UaNode[];
	gateway: Gateway;
	server:Server;
};
@Injectable()
export class NodeResolver implements Resolve<NodePageData> {
	constructor(
		private router:Router,
		private snackbar: SnackbarService,
		@Inject('GatewayService') private gatewayService: GatewayService,
		@Inject('OpcStore') private opcStore:OpcStore
	){}

	async load( route:NodeRoute ):Promise<NodePageData>{
		try{
			let gateway = await this.gatewayService.gateway( route.gatewayTarget );
			const server = await this.opcStore.getConnection( gateway, route.cnnctnTarget );
			const defaultBrowseNs = server.connection.defaultBrowseNs;
			if( !route.node ){
				const vars = { opc: route.cnnctnTarget, path: route.browsePath };
				const node = (await gateway.query<any>(`node( opc: $opc, path:$path ){id name parents{id name path}}`, vars, (m)=>console.log(m)) )["node"];
				if( node.sc )
					throw new EvalError( (await gateway.errorCodeText(node.sc)), {cause:"Opc Interface"} );
				route.node = new OpcObject( {...node, browse: route.browse(defaultBrowseNs)} );
				this.opcStore.insertNode( route, node.parents, defaultBrowseNs );
			}
			let references = await gateway.browseObjectsFolder( route.cnnctnTarget, route.node, true, (m)=>console.log(m) );
			let displayed = references.filter( (r)=>r.displayed );
			this.opcStore.setRoute( route, defaultBrowseNs );
			return { route: route, nodes: displayed, gateway: gateway, server: server };
		}catch( e ){
			this.snackbar.exception( "Not found.", e );
			this.router.navigateByUrl( createUrlTreeFromSnapshot(route.route, ['..']) );//an injected ActivatedRoute is the ROOT route inside a resolver, so relativeTo sent this to '/';  NodeRoute keeps this route's snapshot.
			return undefined as unknown as NodePageData;
		}
	}
	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<NodePageData>{
		return this.load( new NodeRoute(route, this.opcStore) );
	}
}