import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, Route, Router, Routes, UrlSegment} from '@angular/router';
import {Sort} from '@angular/material/sort';
import { MatTable } from '@angular/material/table';
import {IErrorService} from '../../../services/error/IErrorService'
import {IGraphQL, EnumValue } from '../../../services/IGraphQL';
import {Field} from '../../../model/ql/schema/Field';
import {TableSchema}  from '../../../model/ql/schema/TableSchema';
import {MetaObject}  from '../../../model/ql/schema/MetaObject';
import {Settings} from '../../../utils/settings'

import { ComponentPageTitle, DocItem, IRouteService, RouteService } from 'jde-spa';
import { Subject } from 'rxjs';
import { MatIcon } from '@angular/material/icon';
import { MatIconButton } from '@angular/material/button';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatToolbar } from '@angular/material/toolbar';
import { GraphQLTable } from '../../GraphQL/table/table';
import { QLListData, QLListResolver, UserSettings } from '../../../services/ql-list.resolver';
import { SelectionModel } from '@angular/cdk/collections';
import { RouteStore } from '../../../services/route.store';

@Injectable( {providedIn: 'root'} )
export class QLListRouteService extends RouteService implements IRouteService{
	constructor( private router:Router, route: ActivatedRoute ){
		super( route )
	}
	override children( urlSegments:UrlSegment[] ):Promise<Routes>{
		let y:Routes = [];
		let thisConfig = this.router.config.find( x=>x.path==urlSegments[urlSegments.length-1].path );
		let childrenConfig = this.router.config.find( x=>x.path==thisConfig.path && x.children?.length );
		for( let child of childrenConfig.children?.filter(x=> !x.path.endsWith(":target")) ){
			if( child.path!=":collectionDisplay" )
				y.push( child );
			else{
				for( let collection of child.data["collections"] ){
					var route:Route;
					if( typeof collection=='string' ){
						route = {
							title: collection.charAt( 0 ).toUpperCase()+collection.slice(1),
							data: {id: child.path.replace(":collectionDisplay", collection), collectionName:collection},
							path: collection };
					}else{
						const data = collection.data;
						const path = collection.path ?? data.collectionName;
						const upper = path.charAt( 0 ).toUpperCase()+path.slice( 1 );
						route = {
							title: collection.title ?? upper,
							data: data,
							path: child.path.replace(":collectionDisplay", path),
						};
					}
					y.push( route );
				}
			}
		}
		return Promise.resolve( y );
	}
}

@Component( {
		selector: 'ql-list',//.main-content.mat-drawer-container.my-content
		styleUrls: ['ql-list.scss'],
		templateUrl: './ql-list.html',
		host: {class:'main-content mat-drawer-container my-content'},
		imports: [CommonModule, MatCheckbox, MatIcon, MatIconButton, MatToolbar, GraphQLTable]
})
export class QLList implements OnInit, OnDestroy{
	constructor(
		private route: ActivatedRoute,
		private router:Router,
		private componentPageTitle:ComponentPageTitle,
		@Inject('IGraphQL') private ql: IGraphQL,
		@Inject('IErrorService') private snackbar: IErrorService )
	{}
	ngOnDestroy(){ this.profile.save(); }

	async ngOnInit(){
		this.route.data.subscribe( (data)=>{
			if( this.resolvedData )
				this.resolvedData.profile.save();
			this.init( data );
    });
	}
	init( resolvedValue ){
		this.resolvedData = resolvedValue["data"];
		this.data.set( this.resolvedData.data[this.collectionName] );
		this.sideNav.set( this.resolvedData.routing );
		this.resolvedData.data[this.collectionName] = null;
		this.showDeleted = this.resolvedData.profile.value.showDeleted;
		let paths = [];
		for( let x = this.route; x.routeConfig?.data && x.routeConfig?.data["name"]; x = x.parent )
			paths.push( x.routeConfig.data['name'] );
		this.componentPageTitle.title = paths[0];//.join( " | " ); 	//this.componentPageTitle.title ? `${this.componentPageTitle.title} | ${title}` : title;

		const order = ["name", "created", "updated", "deleted", "target", "description"];
		this.displayedFields = Field.filterSort( this.schema.fields, order, [...this.excludedColumns, "description"], this.showDeleted );
		if( !this.excludedColumns.find(x=>x=="description") )
			this.displayedFields.push( this.schema.fields.find(x=>x.name=="description") );

		this.isLoading.set( false );
	};
	sortData( options:Sort ){
		const values = this.data().slice();
		const multiplier = options.direction === 'asc' ? 1 : -1;
		const name = options.active;
		this.data.set( values.sort((a, b) =>{
			let lessThan = a[name]<b[name];
			return (lessThan ? -1 : 1)*multiplier;
		}) );
	}

	edit(){
		if( this.selection().deleted )
			this.ql.mutation( `restore${this.type}("id":${this.selection().id})` ).then( ()=>this.selection().deleted=null ).catch( (e)=>console.log(e) );
		else{
			try{
				this.router.navigate([this.selection().target], {relativeTo: this.route} );
			}catch( e ){
				this.snackbar.exceptionInfo( e, "Could not navigate to properties", (m)=>console.log(m) );
			}
		}
	}

	insert(){
		this.router.navigate(['$new'], {relativeTo: this.route} );
	}

	async delete(){
		const purge = this.selection().deleted!=null;
		const type = purge ? "purge" : "delete";
		try{
			await this.ql.mutation(`${type}${this.type}(id:${this.selection().id})`)
			if( !purge && this.showDeleted )
				this.selection().deleted = new Date();
			else{
				const values = this.data().slice();
				const index = values.findIndex( (x)=>x["id"]==this.selection()["id"] );
				values.splice( index, 1 );
				this.selections.set( new SelectionModel<any>(false, []) );
				this.data.set( values );
			}
		}
		catch( e ){
			this.snackbar.exception( e, (m)=>console.log(m) );
		}
	}
	selection = computed<any>( ()=>{
		return this.selections().selected.length==1 ? this.selections().selected[0] : null;
	});

	async toggleShowDeleted(){
		this.resolvedData.profile.value.showDeleted = !this.showDeleted;
		let reload = await QLListResolver.load( this.ql, this.profile, this.resolvedData.pageSettings, this.resolvedData.routing, this.routeStore );
		this.init( {data:reload} );
	}

	sideNav = model.required<DocItem>();
	collectionDisplay = input.required<string>();

	isLoading = signal<boolean>( true );
	selections = signal<SelectionModel<any>>( new SelectionModel<any>(false, []) );

	displayedFields:Field[];
	@ViewChild('mainTable',{static: false}) _table:MatTable<any>;
	get canPurge():boolean{ return this.resolvedData.pageSettings.canPurge; }
	private get collectionName():string{ return this.schema.collectionName; }
	data = signal<any[]>([]);
	private get excludedColumns(){ return this.resolvedData.pageSettings.excludedColumns; }
	get name():string{ return <string>this.routeConfig.title; }
	get enums():Map<string, EnumValue[]>{ return this.schema.enums; }
	private get profile():Settings<UserSettings>{ return this.resolvedData.profile; }
	resolvedData:QLListData;
	get routeConfig(){ return this.route.routeConfig; }
	routeStore = inject( RouteStore );
	private get schema():TableSchema{ return this.resolvedData.schema; }
	get settings(){ return this.profile.value; }
	get sort(){ return this.settings.sort; }
	showDeleted = false;
	get showAdd():boolean{ return this.resolvedData.pageSettings.showAdd ?? true };
	showDeletedSubject = new Subject<boolean>();
	get type(){ return MetaObject.toTypeFromCollection(this.collectionName);}
}