import {inject, Inject, Injectable} from '@angular/core';
import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import { AppInstanceRoute, IErrorService, PageProfile, PageSettings, QLListResolver, RouteStore, TableSchema, View } from 'jde-framework';
import { Gateway, GatewayService } from '../gateway.service';
import { DocItem, ProfileStore } from 'jde-spa';

export type GatewayData = {
	columns: Record<string,string>;
	pageSettings:PageSettings;
	profile: PageProfile;
	results:{ serverConnections: any }; //{users:ITargetRow[]};
	routing:AppInstanceRoute;
	schema: TableSchema;
};

@Injectable()
export class GatewayResolver implements Resolve<GatewayData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('GatewayService') private gatewayService: GatewayService, @Inject('IErrorService') private cnsl: IErrorService )
	{}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<GatewayData>{
		const routing = new AppInstanceRoute( "gateways", route.params["instance"] );
		routing.siblings = this.routeStore.getChildren( route.parent.url.slice(0, -1) );
		routing.parent = new DocItem( { path: "/apps", title:"Applications" } );
		return this.load( route.params["instance"], routing );
	}

	private async load( instanceName:string, routing:AppInstanceRoute ):Promise<GatewayData>{
		const gateway = await this.gatewayService.gateway( instanceName );
		const pageSettings = new PageSettings(routing);
		const schema = await gateway.schemaWithEnums( "serverConnections", (m)=>console.log(m) );
		var profile = new PageProfile();
		const viewCols = schema.fields.map( f=>f.name );
		const defaultView = new View( {configColumns: viewCols, sort: [{active: "name", direction: "asc"}]}, schema );
		profile.views.push( defaultView );
		await profile.loadViews( schema.collectionName, this.profileStore, schema );
		profile.currentViewIndex = ProfileStore.viewIndex( schema.collectionName );
		profile.showDeleted = ProfileStore.showDeleted( schema.collectionName );

		return GatewayResolver.load( gateway, {columns: QLListResolver.columns(schema, [], []), pageSettings, profile, schema, results: null, routing}, this.routeStore );
	}
	static async load( gateway:Gateway, data:GatewayData, routeStore:RouteStore ):Promise<GatewayData>{
		const query = data.profile.view.query( ProfileStore.showDeleted("gateways") );
		const results = await gateway.query<any>( query.text, query.vars, (m)=>console.log(m) );
		routeStore.setChildren( data.routing.path, results[data.schema.collectionName].map( r=>{return {title:r.name, path: r.target};}) );
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