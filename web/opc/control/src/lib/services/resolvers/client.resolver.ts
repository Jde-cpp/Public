import { ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { DocItem } from 'jde-spa';
import { DetailResolverData, DetailRoute, IErrorService, ListRoute, LocalProfileStore, MetaObject, RouteStore} from 'jde-framework'
import { Gateway, GatewayService } from '../gateway.service';
import { ServerCnnctn } from '../../model/ServerCnnctn';
import { OpcStore } from '../opc-store';
import { Server } from '../../model/Server';

@Injectable()
export class ClientResolver implements Resolve<DetailResolverData<ServerCnnctn>> {
	constructor( private route: ActivatedRoute, private router:Router,
		@Inject('IErrorService') private snackbar: IErrorService,
		@Inject('IGraphQL') private gatewayService: GatewayService
	){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<DetailResolverData<ServerCnnctn>>{
		let collectionDisplay = "Server Connections";
		let target = route.paramMap.get( "connection" );
		return this.loadProfile( route, collectionDisplay, target, state.url );
	}

	private async loadProfile( route: ActivatedRouteSnapshot, collectionDisplay:string, target:string, url:string ):Promise<DetailResolverData<ServerCnnctn>>{
		let gatewayTarget = route.parent.url[route.parent.url.length-1].path;
		const ql = await this.gatewayService.gateway( gatewayTarget );
		let siblings = this.routeStore.getChildren( route.parent.url ).map( s=>new DocItem({path:`${s.path}`, title:s.title}) );
		const routing = new DetailRoute( target, siblings.find(s=>s.path.endsWith('/'+target))?.title, siblings, new DocItem({path:'.', title:route.parent.params["instance"]}) );
		try{
			return ClientResolver.load( ql, this.opcStore, target, routing );
		}
		catch( e ){
			this.snackbar.exceptionInfo( e, `Target not found:  '${target}'`, (m)=>console.log(`${m} - ${e.toString()}`) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
			return null;
		}
	}

	static async load( ql:Gateway, opcStore:OpcStore, target:string, routing:DetailRoute ):Promise<DetailResolverData<ServerCnnctn>>{
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection("serverConnections"), (m)=>console.log(m) );
		let obj = {};
		let server:Server = null;
		if( target && target!="$new" ){
			obj = await ql.querySingle( ql.targetQuery(schema, target, LocalProfileStore.showDeleted('clients')), null, (m)=>console.log(m) );
			for( let query of ql.subQueries(schema.type, obj["id"]) ){
				const subRows = await ql.query<any>( query, null, (m)=>console.log(m) );
				let [property, propValue] = Object.entries(subRows)[0];
				if( !obj[property] )
					obj[property] = [...<[]>propValue];
				else
					obj[property] = obj[property].concat( propValue );
			}
			obj["server"] = await opcStore.getConnection( ql, target );
		}
		return {
			excludedColumns: ql.excludedColumns(schema.collectionName),
			row: obj,
			schema: schema,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
	opcStore:OpcStore = inject( OpcStore );
}