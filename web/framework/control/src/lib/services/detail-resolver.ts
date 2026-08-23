import { ActivatedRouteSnapshot, createUrlTreeFromSnapshot, Resolve, Router, RouterStateSnapshot } from '@angular/router';
import { inject, Inject, Injectable } from '@angular/core';
import { SnackbarService } from '../shared/snackbar/snackbar-service';
import { TableSchema } from '../model/ql/schema/table-schema';
import { IGraphQL } from './graphql';
import { ListRoute, TableSettings } from './ql-list-resolver';
import { MetaObject } from '../model/ql/schema/meta-object';
import { RouteItem, RouteStore } from 'jde-spa';
import { ProfileStore } from 'jde-spa';

export type DetailPageSettings = {
	excludedColumns:string[];
};

export class DetailRoute extends RouteItem{
	constructor( target:string, title:string|undefined, siblings:RouteItem[], parent:RouteItem ){
		super( {path:target, title:title, siblings:siblings, parent:parent} );
		if( parent instanceof ListRoute )//adopt the collection's settings; was never assigned — `routing.tableSettings.excludedColumns` threw and no detail page could resolve
			this.tableSettings = parent.tableSettings;
	}
	tableSettings: TableSettings = { excludedColumns: [] };
}

export type DetailResolverData<T>={
	row:any;
	schema: TableSchema;
	routing:DetailRoute;
};

@Injectable()
export class DetailResolver<T> implements Resolve<DetailResolverData<T>> {
	constructor( private router:Router,
		private snackbar: SnackbarService,
		@Inject('IGraphQL') private ql: IGraphQL
	){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<DetailResolverData<T>>{
		let collectionDisplay = route.url.length>1 ? route.url[route.url.length-2].path : route.data["collectionName"]; //users
		let target = route.paramMap.get( "target" )!;
		return this.loadProfile( route, collectionDisplay, target, state.url );
	}
	private async loadProfile( route: ActivatedRouteSnapshot, collectionDisplay:string, target:string, url:string ):Promise<DetailResolverData<T>>{
		let siblings = this.routeStore.getChildren( collectionDisplay );
		const routing = new DetailRoute( target, siblings.find(s=>s.path.endsWith('/'+target))?.title, siblings,
			ListRoute.find(collectionDisplay, route.parent!.routeConfig!.children!.find(x=>x.path==":collectionDisplay")!.data!["collections"]) );
		try{
			return await DetailResolver.load<T>( this.ql, this.ql.toCollectionName(collectionDisplay), target, routing );//await inside try — without it, async failures skip the catch entirely
		}
		catch( e ){
			this.snackbar.error( `Target not found:  '${target}'` );
			this.router.navigateByUrl( createUrlTreeFromSnapshot(route, ['..']) );//an injected ActivatedRoute is the ROOT route inside a resolver, so relativeTo sent this to '/';  the snapshot is this route.
			return null as unknown as DetailResolverData<T>;
		}
	}

	static async load<T>( ql:IGraphQL, collectionName:string, target:string, routing:DetailRoute ):Promise<DetailResolverData<T>>{
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection(collectionName), (m)=>console.log(m) );
		let obj:any = {};
		if( target!="$new" ){
			obj = await ql.querySingle( ql.targetQuery(schema, target, ProfileStore.showDeleted(collectionName), routing.tableSettings.excludedColumns), {}, (m)=>console.log(m) );
			for( let query of ql.subQueries(schema.type, obj["id"]) ){
				const subRows = await ql.query<any>( query, {}, (m)=>console.log(m) );
				//"acl":[{"role":{"id":33,"name":"Opc Gateway Permissions","deleted":null},"identity":{"id":1}}]}
				let [property, propValue] = Object.entries(subRows)[0];
				if( !obj[property] )
					obj[property] = [...<[]>propValue];
				else
					obj[property] = obj[property].concat( propValue );
			}
		}
		return {
			row: obj,
			schema: schema,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
}