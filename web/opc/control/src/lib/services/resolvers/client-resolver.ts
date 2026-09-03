import { ActivatedRouteSnapshot, createUrlTreeFromSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { RouteItem, RouteStore } from 'jde-spa';
import { DetailResolver, DetailResolverData, DetailRoute, SnackbarService, TargetNotFoundError} from 'jde-framework'
import { Gateway, GatewayService } from '../gateway-service';
import { ServerCnnctn } from '../../model/server-cnnctn';
import { OpcStore } from '../opc-store';

@Injectable()
export class ClientResolver implements Resolve<DetailResolverData<ServerCnnctn>> {
	private router:Router = inject( Router );
	//still constructor injection: 'IGraphQL' is a STRING token, which inject() cannot take (review3 C5)
	constructor( @Inject('IGraphQL') private gatewayService: GatewayService ){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<DetailResolverData<ServerCnnctn>>{
		let collectionDisplay = "Server Connections";
		let target = route.paramMap.get( "connection" )!;
		return this.loadProfile( route, collectionDisplay, target, state.url );
	}

	private async loadProfile( route: ActivatedRouteSnapshot, collectionDisplay:string, target:string, url:string ):Promise<DetailResolverData<ServerCnnctn>>{
		const parent = route.parent!;
		let gatewayTarget = parent.url[parent.url.length-1].path;
		const ql = await this.gatewayService.gateway( gatewayTarget );
		let siblings = this.routeStore.getChildren( parent.url ).map( s=>new RouteItem({path:`${s.path}`, title:s.title}) );
		const routing = new DetailRoute(
			target,
			siblings.find(s=>s.path==target || s.path.endsWith('/'+target))?.title,
			siblings,
			new RouteItem({path:'.', title:parent.params["instance"]})
		);
		try{
			return await ClientResolver.load( ql, this.opcStore, target, routing, this.snackbar );//await inside try — without it, async failures skip the catch entirely
		}
		catch( e:unknown ){
			//As DetailResolver:  a missing row and a failed query used to arrive here as the same throw - the server answers
			//{"data":{"serverConnection":null}} for a target it does not have, and the null then TypeError'd on obj["id"] -
			//so a malformed query or a 500 was reported as "Target not found." and the real error never left the log.
			if( e instanceof TargetNotFoundError )
				this.snackbar.error( e.message );
			else
				this.snackbar.exception( `Could not load '${target}'`, e );
			this.router.navigateByUrl( createUrlTreeFromSnapshot(route, ['..']) );//an injected ActivatedRoute is the ROOT route inside a resolver, so relativeTo sent this to '/';  the snapshot is this route.
			return null as unknown as DetailResolverData<ServerCnnctn>;
		}
	}

	//Delegates to DetailResolver.load instead of copying it (review3 C1).  The copy had already drifted - L14's null guard
	//had to be written here a second time - and the opcStore fetch below is the only part that was ever Client-specific.
	//`null` vars, not the default {}: that is what this query has always sent to the gateway.
	static async load( ql:Gateway, opcStore:OpcStore, target:string, routing:DetailRoute, snackbar:SnackbarService ):Promise<DetailResolverData<ServerCnnctn>>{//snackbar is passed in:  inject() needs an injection context, which a static method never has.
		const y = await DetailResolver.load<ServerCnnctn>( ql, "serverConnections", target, routing, null );
		if( target && target!="$new" ){
			try{
				y.row["server"] = await opcStore.getConnection( ql, target );
			}
			catch( e ){ //can't connect, maybe bad settings.
				snackbar.exception( "Could not connect to server.", e );
			}
		}
		return y;
	}
	opcStore:OpcStore = inject( OpcStore );
	routeStore = inject( RouteStore );
	snackbar = inject( SnackbarService );
}