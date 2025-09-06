import { Inject, Injectable } from '@angular/core';
import { ActivatedRoute, ActivatedRouteSnapshot, Params, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { IErrorService, IProfile, Settings, TableSchema } from 'jde-framework';
import { GatewayService } from '../gateway.service';
import { ENodeClass, UaNode } from '../../model/Node';
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