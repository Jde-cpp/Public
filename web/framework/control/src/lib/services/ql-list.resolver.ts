import {ActivatedRoute, ActivatedRouteSnapshot, Resolve, Router, RouterStateSnapshot} from '@angular/router';
import {inject, Inject, Injectable} from '@angular/core';
import {IErrorService} from './error/IErrorService';
import { TableSchema} from '../model/ql/schema/TableSchema';
import { IGraphQL } from '../services/IGraphQL';
import { PageSettings } from '../pages/GraphQL/model/PageSettings';
import { StringUtils } from '../utils/StringUtils';
import { MetaObject } from '../model/ql/schema/MetaObject';
import { Field } from '../model/ql/schema/Field';
import { Sort } from '@angular/material/sort';
import { DocItem } from 'jde-spa';
import { RouteStore } from './route.store';
import { LocalProfileStore } from 'jde-framework';

export type CollectionItem = string | { path:string, title?:string, data?:{summary:string, collectionName:string, canPurge?:boolean,showAdd?:boolean} };
export class ListRoute extends DocItem{
	constructor( collection:string|CollectionItem ){
		super();
		if( typeof collection=='string' )
			collection = {path:collection, title:StringUtils.capitalize(collection)};
		this.path = collection.path;
		this.collectionName = collection.data?.collectionName ?? this.path;
		//this.excludedColumns = collection.data ? collection.data["excludedColumns"] : undefined;
		this.canPurge = collection.data?.canPurge ?? true;
		this.showAdd = collection.data?.showAdd ?? true;
		this.summary = collection?.data?.summary;
		this.title = collection.title ?? StringUtils.capitalize( this.path );
	}
	static find( target:string, collections:CollectionItem[] ):ListRoute{
		let collection:CollectionItem = collections.find( c=>((typeof c =="string") && c==target) || c["path"]==target );
		return new ListRoute( collection );
	}

	canPurge?: boolean; //if true, the user can purge deleted items.
	collectionName: string;
	showAdd?: boolean; //if true, the user can add new items.
}

export type QLListData = {
	showDeleted:boolean;
	pageSettings:PageSettings;
	schema: TableSchema;
	data: any; //{users:ITargetRow[]};
	routing:ListRoute;
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
		routing.excludedColumns = this.ql.excludedColumns( routing.collectionName );
		return QLListResolver.load( this.ql, new PageSettings(routing), routing, this.routeStore );
	}
	static async load( ql:IGraphQL, pageSettings:PageSettings, routing:ListRoute, routeStore:RouteStore ):Promise<QLListData>{
		const collectionName = routing.collectionName;
		const schema = await ql.schemaWithEnums( MetaObject.toTypeFromCollection(collectionName) );
		const showDeleted = LocalProfileStore.showDeleted( collectionName );
		let columns = Field.filter( schema.fields, pageSettings.excludedColumns, showDeleted ).map( x=>x.name );
		let query = `${collectionName}{ ${columns.join(" ")} }`;
		const data = await ql.query<any>( query );
		routeStore.setChildren( routing.path, data[schema.collectionName].map( r=>{return {title:r.name, path:`${routing.path}/${r.target}`};}) );

		return {
			showDeleted: showDeleted,
			pageSettings: pageSettings,
			schema: schema,
			data: data,
			routing: routing
		};
	}
	routeStore = inject( RouteStore );
}