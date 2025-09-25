import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import {inject, Inject, Injectable} from '@angular/core';
import { IErrorService, Field, ListRoute, PageSettings, IProfile, QLListData, RouteStore, Settings, UserSettings } from 'jde-framework';
import { Gateway, GatewayService, GatewayTarget } from '../gateway.service';

@Injectable()
export class ConnectionResolver implements Resolve<QLListData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('GatewayService') private gatewayService: GatewayService, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private cnsl: IErrorService )
	{}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<QLListData>{
		const host = route.paramMap.get( "host" );
		const config = route.routeConfig;
		const data = config.data;
		const routing = new ListRoute( {path:route.parent.url.join('/'), title: host as string, data:{summary: "", collectionName: "serverConnections"}} );
		routing.siblings = this.routeStore.getSiblings( route.parent.url.slice(0, -1) );
		return this.load( route.params["gateway"], routing );
	}

	private async load( gatewayTarget:GatewayTarget, routing:ListRoute ):Promise<QLListData>{
		const profile = new Settings<UserSettings>( UserSettings, `${routing.collectionName}`, this.profileService );
		await profile.loadedPromise;
		const gateway = await this.gatewayService.gateway( gatewayTarget );
		routing.excludedColumns = gateway.excludedColumns( routing.collectionName );
		return ConnectionResolver.load( gateway, profile, new PageSettings(routing), routing, this.routeStore );
	}
	static async load( gateway:Gateway, profile:Settings<UserSettings>, pageSettings:PageSettings, routing:ListRoute, routeStore:RouteStore ):Promise<QLListData>{
		const schema = await gateway.schemaWithEnums( "serverConnections", console.log );
		let columns = Field.filter( schema.fields, pageSettings.excludedColumns, profile.value.showDeleted ).map( x=>x.name );
		let query = `serverConnections{ ${columns.join(" ")} }`;
		const data = await gateway.query<any>( query );
		routeStore.setSiblings( routing.path, data[schema.collectionName].map( r=>{return {title:r.name, path: r.target};}) );

		return {
			profile: profile,
			pageSettings: pageSettings,
			schema: schema,
			data: data,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
}