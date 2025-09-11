import { Inject, Injectable } from '@angular/core';
import { ActivatedRoute, ActivatedRouteSnapshot, Params, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { IErrorService, IProfile, Settings, TableSchema } from 'jde-framework';
import { GatewayService } from '../gateway.service';
import { OpcObject, UaNode } from '../../model/Node';
import { NodeRoute, UserProfile } from '../../model/NodeRoute';
import { OpcStore } from '../opc-store';

export type NodePageData = {
	route:NodeRoute;
	nodes:UaNode[];
};
@Injectable()
export class NodeResolver implements Resolve<NodePageData> {
	constructor(
		private router:Router,
		@Inject('IProfile') private profileService: IProfile,
		@Inject('IErrorService') private snackbar: IErrorService,
		@Inject('GatewayService') private gatewayService: GatewayService,
		@Inject('OpcStore') private opcStore:OpcStore,
		@Inject(ActivatedRoute) private route: ActivatedRoute
	){}

	async load( route:NodeRoute ):Promise<NodePageData>{
		try{
			await route.settings.loadedPromise;//this.route.snapshot.children[0].paramMap.get('host')
			let gateway = await this.gatewayService.instance( route.gatewayTarget );
			if( !route.node ){
				const cnnctn = await this.opcStore.getConnection( gateway, route.cnnctnTarget );
				let segment = route.browsePath.substring( route.browsePath.lastIndexOf("/")+1 );
				route.node = new OpcObject( {
					...(await gateway.query<any>( `node( opc: "${route.cnnctnTarget}", path:"${route.browsePath}"){id name}`))["node"],
					browseName: { ns: cnnctn.getNs(segment), name: cnnctn.removeNs(segment) }
				} );
				if( route.browsePath.split("/").length==1 ){
					route.node.parent = OpcObject.rootNode;
					this.opcStore.insertNode( route.gatewayTarget, route.cnnctnTarget, route.node );
				}
			}
			let references = await gateway.browseObjectsFolder( route.cnnctnTarget, route.node, true, (m)=>console.log(m) );
			let displayed = references.filter( (r)=>r.displayed );
			this.opcStore.setRoute( route );
			return { route: route, nodes: displayed };
		}catch( e ){
			this.snackbar.exceptionInfo( e, `Not found:  '${route.cnnctnTarget}'`, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
			return null;
		}
	}
	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<NodePageData>{
		return this.load( new NodeRoute(route, this.profileService, this.opcStore) );
	}
}