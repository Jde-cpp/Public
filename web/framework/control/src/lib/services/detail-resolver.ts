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

//A target the server does not have, told apart from a query that failed.  Both used to arrive here as the same throw - the
//server answers {"data":{"role":null}} for a missing row and the null then TypeError'd on obj["id"] in the subQueries loop -
//so a malformed query (an unknown column, a 500) was reported as "Target not found" and the real error never left the log.
export class TargetNotFoundError extends Error{
	constructor( readonly target:string ){
		super( `Target not found:  '${target}'` );
		this.name = "TargetNotFoundError";
	}
}

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
		//ComponentNav renders each sibling as parent.path + '/' + sibling.path, so the parent must be the absolute list url
		//('/access/users') and the siblings bare targets — the relative ListRoute path resolved against the sidenav route
		//('/access/users/users/<target>'), breaking sibling navigation and the routerLinkActive highlight.
		let siblings = this.routeStore.getChildren( collectionDisplay ).map( s=>new RouteItem(
			{path: s.path.startsWith(collectionDisplay+'/') ? s.path.substring(collectionDisplay.length+1) : s.path, title: s.title}) );//pre-fix localStorage entries are collection-prefixed
		const parent = ListRoute.find( collectionDisplay, route.parent!.routeConfig!.children!.find(x=>x.path==":collectionDisplay")!.data!["collections"] );
		parent.path = `/${[...route.parent!.url.map(s=>s.path), collectionDisplay].join('/')}`;
		const routing = new DetailRoute( target, siblings.find(s=>s.path==target)?.title, siblings, parent );
		try{
			return await DetailResolver.load<T>( this.ql, this.ql.toCollectionName(collectionDisplay), target, routing );//await inside try — without it, async failures skip the catch entirely
		}
		catch( e ){
			if( e instanceof TargetNotFoundError )
				this.snackbar.error( e.message );
			else
				this.snackbar.exception( `Could not load '${target}'`, e );//whatever actually failed - a 500 from a malformed query used to be indistinguishable from a missing row
			this.router.navigateByUrl( createUrlTreeFromSnapshot(route, ['..']) );//an injected ActivatedRoute is the ROOT route inside a resolver, so relativeTo sent this to '/';  the snapshot is this route.
			return null as unknown as DetailResolverData<T>;
		}
	}

	//`vars` is a parameter only so ClientResolver can share this body without changing what goes on the wire (review3 C1):
	//ql() appends `&variables=` for any TRUTHY vars, so the {} the access pages have always sent is not the same request as
	//the null the gateway has always had, and the two talk to different servers.  Neither behaviour is proven on the other.
	static async load<T>( ql:IGraphQL, collectionName:string, target:string, routing:DetailRoute, vars:any={} ):Promise<DetailResolverData<T>>{
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection(collectionName), (m)=>console.log(m) );
		let obj:any = {};
		if( target && target!="$new" ){//`target &&`: a missing route param must not query for the row named 'undefined'
			obj = await ql.querySingle( ql.targetQuery(schema, target, ProfileStore.showDeleted(collectionName), routing.tableSettings.excludedColumns), vars, (m)=>console.log(m) );
			if( obj==null )//{"data":{"<singular>":null}} - the row is not there.  Checked before the subQueries loop, whose obj["id"] would otherwise TypeError and hide every other failure behind the same message.
				throw new TargetNotFoundError( target );
			for( let query of ql.subQueries(schema.type, obj["id"]) ){
				const subRows = await ql.query<any>( query, vars, (m)=>console.log(m) );
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