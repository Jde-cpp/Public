import { inject, Injectable } from '@angular/core';
import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot, UrlSegment} from '@angular/router';
import { AppInstanceRoute, SnackbarService, PageProfile, PageSettings, QLListResolver, TableSchema, View } from 'jde-framework';
import { Gateway, GATEWAY_SERVICE, GatewayService } from '../gateway-service';
import { RouteItem, ProfileStore, RouteStore } from 'jde-spa';

export type GatewayData = {
	columns: Record<string,string>;
	pageSettings:PageSettings;
	profile: PageProfile;
	results:{ serverConnections: any }|undefined; //{users:ITargetRow[]};
	routing:AppInstanceRoute;
	schema: TableSchema;
};

@Injectable()
export class GatewayResolver implements Resolve<GatewayData> {
	private route:ActivatedRoute = inject( ActivatedRoute );
	private router:Router = inject( Router );
	private cnsl:SnackbarService = inject( SnackbarService );
	private gatewayService:GatewayService = inject( GATEWAY_SERVICE );

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<GatewayData>{
		const routing = new AppInstanceRoute( "gateways", route.params["instance"], route.data["tableSettings"] );
		routing.siblings = this.routeStore.getChildren( route.parent!.url.slice(0, -1) );
		routing.parent = new RouteItem( { path: "/apps", title:"Applications" } );
		return this.load( route.params["instance"], routing, route.parent!.url );
	}

	private async load( instanceName:string, routing:AppInstanceRoute, url:UrlSegment[] ):Promise<GatewayData>{
		const gateway = await this.gatewayService.gateway( instanceName );
		const pageSettings = new PageSettings( routing.tableSettings );
		const schema = await gateway.schemaWithEnums( "ServerConnection", (m)=>console.log(m) );
		var profile = new PageProfile();
		const defaultView = new View( routing.tableSettings, schema );
		profile.views.push( defaultView );
		await profile.loadViews( schema.collectionName, this.profileStore, schema, defaultView.sort );
		profile.currentViewIndex = ProfileStore.viewIndex( schema.collectionName );
		profile.showDeleted = ProfileStore.showDeleted( schema.collectionName );

		return GatewayResolver.load( gateway, {columns: QLListResolver.columns(schema, [], []), pageSettings, profile, schema, results: undefined, routing}, this.routeStore, url );
	}
	static async load( gateway:Gateway, data:GatewayData, routeStore:RouteStore, childrenKey:string|UrlSegment[] ):Promise<GatewayData>{
		const query = data.profile.view.query( data.profile.showDeleted, 0 );//the toggle persists under the collection name (serverConnections), not "gateways"
		const results = await gateway.query<any>( query.text, query.vars, (m)=>console.log(m) );
		routeStore.setChildren( childrenKey, results[data.schema.collectionName].map( (r:any)=>{return {title:r.name, path: r.target};}) );
		return {
			columns: data.columns,
			pageSettings: data.pageSettings,
			profile: data.profile,
			results: results,
			routing: data.routing,
			schema: data.schema
		};
	}
	routeStore = inject( RouteStore );
	profileStore = inject( ProfileStore );
}