import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import {inject, Inject, Injectable} from '@angular/core';
import {IErrorService} from './error/IErrorService';
import { TableSchema} from '../model/ql/schema/TableSchema';
import { IGraphQL } from '../services/IGraphQL';
import { PageProfile, PageSettings } from '../pages/GraphQL/model/PageSettings';
import { StringUtils } from '../utils/StringUtils';
import { MetaObject } from '../model/ql/schema/MetaObject';
import { Field } from '../model/ql/schema/Field';
import { DocItem, ProfileStore } from 'jde-spa';
import { RouteStore } from './route.store';
import { View, ViewFieldSettings } from '../model/ql/View';
import { Sort } from '@angular/material/sort';

export type TableSettings = {canPurge?:boolean,showAdd?:boolean, excludedColumns:string[], columns?:(string|ViewFieldSettings)[], sort?:Sort[]};
export type CollectionItem = string | { path:string, title?:string, data?:{summary:string, collectionName:string, tableSettings:TableSettings} };
export class ListRoute extends DocItem{
	constructor( collection:string|CollectionItem ){
		super();
		if( typeof collection=='string' )
			collection = {path:collection, title:StringUtils.capitalize(collection)};
		this.path = collection.path;
		this.collectionName = collection.data?.collectionName ?? this.path;
		this.tableSettings = collection.data?.tableSettings ?? { excludedColumns: [], columns: ["name", "created", "updated", "deleted", "target"], sort: [{active:"name", direction:"asc"}] };
		this.summary = collection?.data?.summary;
		this.title = collection.title ?? StringUtils.capitalize( this.path );
	}
	static find( target:string, collections:CollectionItem[] ):ListRoute{
		let collection:CollectionItem = collections.find( c=>((typeof c =="string") && c==target) || c["path"]==target );
		return new ListRoute( collection );
	}
	tableSettings:TableSettings;
	collectionName: string;
}

export type QLListData = {
	columns: Record<string,string>;
	pageSettings:PageSettings;
	profile: PageProfile;
	results: any; //{users:ITargetRow[]};
	routing:ListRoute;
	schema: TableSchema;
};

@Injectable()
export class QLListResolver implements Resolve<QLListData> {
	constructor( private route: ActivatedRoute, private router:Router, @Inject('IGraphQL') private ql: IGraphQL, @Inject('IErrorService') private cnsl: IErrorService ){}

	resolve(route: ActivatedRouteSnapshot, state: RouterStateSnapshot):Promise<QLListData>{
		const collectionDisplay = route.paramMap.get( "collectionDisplay" );
		//let parent = { path: route.parent.url.map(seg=>seg.path).join("/"), title: route.parent.title ?? StringUtils.capitalize(route.parent.url[route.parent.url.length-1].path) };
		let routing:ListRoute;
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
		//routing.parent = parent;
		return this.load( routing );
	}

	private async load( routing:ListRoute ):Promise<QLListData>{
		let pageSettings = new PageSettings( routing.tableSettings );
		const collectionName = routing.collectionName;
		const schema = await this.ql.schemaWithEnums( MetaObject.toTypeFromCollection(collectionName), (m)=>console.log(m) );
		let defaultView = await QLListResolver.defaultView( schema, pageSettings.configColumns );
		var profile = new PageProfile();
		profile.views.push( defaultView );
		await profile.loadViews( collectionName, this.profileStore, schema );
		profile.currentViewIndex = ProfileStore.viewIndex( collectionName );
		profile.showDeleted = ProfileStore.showDeleted( collectionName );
		return QLListResolver.load( this.ql, {pageSettings, profile, schema, results: null, routing, columns: QLListResolver.columns(schema, routing.tableSettings.columns, routing.tableSettings.excludedColumns)}, this.routeStore );
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
	static async load( ql:IGraphQL, data:QLListData, routeStore:RouteStore ):Promise<QLListData>{
		const q = data.profile.view.query( data.profile.showDeleted );
		data.results = await ql.query<any>( q.text, q.vars, (m)=>console.log(m) );
		const children = data.results[data.schema.collectionName].map( r=>({title:r.name, path:`${data.routing.path}/${r.target}`}) );
		routeStore.setChildren( data.routing.path, children );
		return {
			columns: data.columns,
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