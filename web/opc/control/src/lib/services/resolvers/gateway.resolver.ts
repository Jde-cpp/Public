import {inject, Inject, Injectable} from '@angular/core';
import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import { AppInstanceRoute, IErrorService, PageSettings, RouteStore, TableSchema, View } from 'jde-framework';
import { Gateway, GatewayService } from '../gateway.service';
import { DocItem, ProfileStore } from 'jde-spa';

export type GatewayData = {
	pageSettings:PageSettings;
	schema: TableSchema;
	data:{ serverConnections: any }; //{users:ITargetRow[]};
	routing:AppInstanceRoute;
};

@Injectable()
export class GatewayResolver implements Resolve<GatewayData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('GatewayService') private gatewayService: GatewayService, @Inject('IErrorService') private cnsl: IErrorService )
	{}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<GatewayData>{
		const host = route.paramMap.get( "host" );
		const data = route.routeConfig.data;
		let parent = new DocItem( { path: "/apps", title:"Applications" } );
		const routing = new AppInstanceRoute( "gateways", route.params["instance"] );
		routing.siblings = this.routeStore.getChildren( route.parent.url.slice(0, -1) );
		routing.parent = parent;
		return this.load( route.params["instance"], routing );
	}

	private async load( instanceName:string, routing:AppInstanceRoute ):Promise<GatewayData>{
		const gateway = await this.gatewayService.gateway( instanceName );
		//routing.excludedColumns = gateway.excludedColumns( "serverConnections" );
		return GatewayResolver.load( gateway, new PageSettings(routing), routing, this.routeStore );
	}
	static async load( gateway:Gateway, pageSettings:PageSettings, routing:AppInstanceRoute, routeStore:RouteStore ):Promise<GatewayData>{
		const schema = await gateway.schemaWithEnums( "serverConnections", console.log );
		const viewCols = schema.fields.map( f=>f.name );
		const view = new View( {configColumns: viewCols, sort: [{active: "name", direction: "asc"}]}, schema );
		const query = view.query( ProfileStore.showDeleted("gateways") );
		const data = await gateway.query<any>( query.text, query.vars, console.log );
		routeStore.setChildren( routing.path, data[schema.collectionName].map( r=>{return {title:r.name, path: r.target};}) );

		return {
			pageSettings: pageSettings,
			schema: schema,
			data: data,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
}