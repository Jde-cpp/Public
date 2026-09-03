import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import {inject, Inject, Injectable} from '@angular/core';
import {SnackbarService} from '../shared/snackbar/snackbar-service';
import { TableSchema} from '../model/ql/schema/table-schema';
import { IGraphQL } from './graphql';
import { PageProfile, PageSettings } from '../pages/graphql/model/page-settings';
import { StringUtils } from '../utils/string-utils';
import { MetaObject } from '../model/ql/schema/meta-object';
import { Field } from '../model/ql/schema/field';
import { RouteItem, ProfileStore, RouteStore } from 'jde-spa';
import { FieldFilter, View, ViewFieldSettings } from '../model/ql/view';
import { Sort } from '@angular/material/sort';

export type TableSettings = {canPurge?:boolean,canAdd?:boolean, canNavigate?:boolean, excludedColumns?:string[], columns?:(string|ViewFieldSettings)[], sort?:Sort[]|string};//canNavigate: a collection with no ':target' detail route must not offer the row click-through
export type CollectionItem = string | { path:string, title?:string, data?:{summary:string, collectionName:string, tableSettings:TableSettings} };
export class ListRoute extends RouteItem{
	constructor( collection:string|CollectionItem ){
		super();
		if( typeof collection=='string' )
			collection = {path:collection, title:StringUtils.capitalize(collection)};
		this.path = collection.path;
		this.collectionName = collection.data?.collectionName ?? this.path;
		//per-field defaults: routes pass partial settings (e.g. groups/roles set only excludedColumns) and columns must never be undefined.
		this.tableSettings = { excludedColumns: [], columns: ["name", "created", "updated", "deleted", "target"], sort: [{active:"name", direction:"asc"}], ...collection.data?.tableSettings };
		this.summary = collection?.data?.summary;
		this.title = collection.title ?? StringUtils.capitalize( this.path );
	}
	static find( target:string, collections:CollectionItem[] ):ListRoute{
		const collection = collections.find( (c:any)=>((typeof c =="string") && c==target) || c["path"]==target );
		return new ListRoute( collection ?? target );//unlisted collection - the defaults (QLSelector embeds collections its host's route never declares)
	}
	tableSettings:TableSettings;
	collectionName: string;
}

export type QLListData = {
	columns: Record<string,string>;
	fixedFilters?: FieldFilter[];//applied on top of whichever view is current and never shown or saved - QLSelector's excludedIds
	pageSettings:PageSettings;
	profile: PageProfile;
	results: any; //{users:ITargetRow[]};
	routing:ListRoute;
	schema: TableSchema;
};

@Injectable()
export class QLListResolver implements Resolve<QLListData> {
	private route:ActivatedRoute = inject( ActivatedRoute );
	private router:Router = inject( Router );
	private cnsl:SnackbarService = inject( SnackbarService );
	constructor( @Inject('IGraphQL') private ql: IGraphQL ){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<QLListData>{
		const collectionDisplay = route.paramMap.get( "collectionDisplay" );
		let routing:ListRoute|undefined;
		const siblings:ListRoute[] = [];
		for( let collection of route.data["collections"] ){
			const sibling = new ListRoute( collection );
			siblings.push( sibling );
			if( sibling.path==collectionDisplay )
				routing = sibling;
		}
		if( !routing )
			routing = siblings[0];
		routing.siblings = siblings;
		routing.parent = QLListResolver.parentRoute( route );
		if( routing.parent )
			this.routeStore.setChildren( routing.parent.path, siblings );//the breadcrumb resolves a ':collectionDisplay' segment through RouteStore;  unregistered, it fell back to the raw url segment ('users' instead of 'Users')
		return this.load( routing );
	}

	//the sidenav header link back to the collection's landing page ('/access' for access/users).  ComponentNav renders the
	//siblings as parent.path + '/' + sibling.path, so the path must be absolute or every sibling resolves relative to the
	//current url instead.
	private static parentRoute( route:ActivatedRouteSnapshot ):RouteItem|undefined{
		const segments = route.parent?.url.map( seg=>seg.path ) ?? [];
		if( !segments.length )
			return undefined;
		const title = route.parent!.title ?? StringUtils.capitalize( segments[segments.length-1] );
		return new RouteItem( {path: `/${segments.join('/')}`, title} );
	}

	private async load( routing:ListRoute ):Promise<QLListData>{
		return QLListResolver.load( this.ql, await QLListResolver.data(this.ql, routing, this.profileStore), this.routeStore );
	}
	//everything but the rows:  the schema, the default view plus the user's saved ones, and the column display names.
	//QLSelector builds a collection's list the same way, so it lives here rather than in resolve().
	static async data( ql:IGraphQL, routing:ListRoute, profileStore:ProfileStore ):Promise<QLListData>{
		let pageSettings = new PageSettings( routing.tableSettings );
		const collectionName = routing.collectionName;
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection(collectionName), (m)=>console.log(m) );
		let defaultView = await QLListResolver.defaultView( schema, pageSettings.configColumns );
		var profile = new PageProfile();
		profile.views.push( defaultView );
		await profile.loadViews( collectionName, profileStore, schema, defaultView.sort );
		profile.currentViewIndex = ProfileStore.viewIndex( collectionName );
		profile.showDeleted = ProfileStore.showDeleted( collectionName );
		return {pageSettings, profile, schema, results: null, routing, columns: QLListResolver.columns(schema, routing.tableSettings.columns!, routing.tableSettings.excludedColumns)};
	}
	private static async defaultView( schema:TableSchema, configColumns:(string|ViewFieldSettings)[] ):Promise<View>{
		let defaultView = new View( {configColumns: configColumns, sort: [{active: "name", direction: "asc"}]}, schema );
		return defaultView;
	}
	static columns( schema:TableSchema, configColumns:(string|ViewFieldSettings)[], excluded: string[] ):Record<string,string>{
		let columns: Record<string,string> = {};
		for( let field of schema.fields.filter(f=>!excluded.includes(f.name)) ){
			let configColumn = configColumns.find( c=>typeof c=="object" && c.name==field.name ) as ViewFieldSettings;
			columns[field.name] = configColumn?.displayName ?? StringUtils.idToDisplay(field.name);
		}
		return columns;
	}
	//routeStore null:  the rows are a pick list (QLSelector - filtered, and never the page's own collection), not the collection's sidenav children
	static async load( ql:IGraphQL, data:QLListData, routeStore:RouteStore|null ):Promise<QLListData>{
		let view = data.profile.view;
		if( data.fixedFilters?.length ){//on a copy:  the profile's view is what the settings panel edits and what a Save persists
			view = new View( view );
			view.fieldFilters = [...view.fieldFilters, ...data.fixedFilters];
		}
		const q = view.query( data.profile.showDeleted, 0 );
		data.results = await ql.query<any>( q.text, q.vars, (m)=>console.log(m) );
		if( routeStore ){
			const children = data.results[data.schema.collectionName].map( (r:any)=>({title:r.name, path:`${r.target}`}) );//bare targets, like GatewayResolver — DetailResolver renders them under the absolute list url
			routeStore.setChildren( data.routing.path, children );
		}
		return {
			columns: data.columns,
			fixedFilters: data.fixedFilters,
			pageSettings: data.pageSettings,
			results: data.results,
			routing: data.routing,
			profile: data.profile,
			schema: data.schema
		};
	}
	routeStore = inject( RouteStore );
	profileStore = inject( ProfileStore );
}