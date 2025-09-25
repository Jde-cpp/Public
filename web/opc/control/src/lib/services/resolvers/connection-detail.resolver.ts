import { ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { DocItem } from 'jde-spa';
import { DetailResolverData, DetailRoute, IErrorService, ListRoute, MetaObject, IProfile, RouteStore, Settings, UserSettings} from 'jde-framework'
import { Gateway, GatewayService } from '../gateway.service';
import { ServerCnnctn } from '../../model/ServerCnnctn';

@Injectable()
export class CnnctnDetailResolver implements Resolve<DetailResolverData<ServerCnnctn>> {
	constructor( private route: ActivatedRoute, private router:Router,
		@Inject('IProfile') private profileService: IProfile,
		@Inject('IErrorService') private snackbar: IErrorService,
		@Inject('IGraphQL') private gatewayService: GatewayService
	){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<DetailResolverData<ServerCnnctn>>{
		let collectionDisplay = "Server Connections";
		let target = route.paramMap.get( "target" );
		return this.loadProfile( route, collectionDisplay, target, state.url );
	}

	private async loadProfile( route: ActivatedRouteSnapshot, collectionDisplay:string, target:string, url:string ):Promise<DetailResolverData<ServerCnnctn>>{
		let gatewayTarget = route.parent.url[route.parent.url.length-1].path;
		this.ql = await this.gatewayService.gateway( gatewayTarget );
		const profile = new Settings<UserSettings>( UserSettings, `serverConnections-detail`, this.profileService );
		await profile.loadedPromise;

		let siblings = this.routeStore.getSiblings( route.parent.url ).map( s=>new DocItem({path:`${gatewayTarget}/${s.path}`, title:s.title}) );
		const routing = new DetailRoute( target, siblings.find(s=>s.path.endsWith('/'+target))?.title, siblings, new ListRoute(gatewayTarget) );
		try{
			return CnnctnDetailResolver.load( this.ql, "serverConnections", target, profile, routing );
		}
		catch( e ){
			this.snackbar.exceptionInfo( e, `Target not found:  '${target}'`, (m)=>console.log(`${m} - ${e.toString()}`) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
			return null;
		}
	}

	static async load( ql:Gateway, collectionName:string, target:string, profile:Settings<UserSettings>, routing:DetailRoute ):Promise<DetailResolverData<ServerCnnctn>>{
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection("serverConnections"), (m)=>console.log(m) );
		let obj = {};
		if( target!="$new" ){
			obj = await ql.querySingle( ql.targetQuery(schema, target, profile.value.showDeleted), (m)=>console.log(m) );
			for( let query of ql.subQueries(schema.type, obj["id"]) ){
				const subRows = await ql.query<any>( query, (m)=>console.log(m) );
				let [property, propValue] = Object.entries(subRows)[0];
				if( !obj[property] )
					obj[property] = [...<[]>propValue];
				else
					obj[property] = obj[property].concat( propValue );
			}
		}
		return {
			pageSettings: {profile:profile,excludedColumns:ql.excludedColumns(schema.collectionName)},
			row: obj,
			schema: schema,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
	ql:Gateway;
}