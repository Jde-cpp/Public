import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, Route, Router, Routes, UrlSegment} from '@angular/router';
import {Sort} from '@angular/material/sort';
import { MatTable } from '@angular/material/table';
import {FormsModule} from '@angular/forms';
import {MatSelectModule} from '@angular/material/select';
import { QLListSettings } from './ql-list-settings/ql-list-settings';
import {IErrorService} from '../../../services/error/IErrorService'
import {IGraphQL, EnumValue } from '../../../services/IGraphQL';
import {Field} from '../../../model/ql/schema/Field';
import {TableSchema}  from '../../../model/ql/schema/TableSchema';
import {MetaObject}  from '../../../model/ql/schema/MetaObject';

import { ComponentPageTitle, DocItem, IRouteService, RouteService } from 'jde-spa';
import { MatIcon } from '@angular/material/icon';
import { MatIconButton, MatAnchor } from '@angular/material/button';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatToolbar } from '@angular/material/toolbar';
import { ProfileStore } from 'jde-spa';
import { GraphQLTable } from '../../GraphQL/table/table';
import { QLListData, QLListResolver, TableSettings } from '../../../services/ql-list.resolver';
import { SelectionModel } from '@angular/cdk/collections';
import { RouteStore } from '../../../services/route.store';
import { View, ViewField, ViewType } from '../../../model/ql/View';
import { PageProfile } from '../../GraphQL/model/PageSettings';
import { MatSelect } from "@angular/material/select";
import { assert } from '../../../utils/utils';

@Component({
	selector: 'ql-list',//.main-content.mat-drawer-container.my-content
	styleUrls: ['ql-list.scss'],
	templateUrl: './ql-list.html',
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, FormsModule, GraphQLTable, MatAnchor, MatCheckbox, MatIcon, MatIconButton, MatSelectModule, MatToolbar, QLListSettings]
})
export class QLList implements OnInit, OnDestroy{
	constructor(
		private route: ActivatedRoute,
		private router:Router,
		private componentPageTitle:ComponentPageTitle,
		@Inject('IGraphQL') private ql: IGraphQL,
		@Inject('IErrorService') private snackbar: IErrorService)
	{}

	ngOnDestroy(){
		//this.profileStore.save(this.collectionName(), this.profile);
		ProfileStore.setShowDeleted( this.collectionName(), this.showDeleted() );
	}

	async ngOnInit(){
		this.route.data.subscribe( (data)=>{
			this.init( data );
    });
	}
	async init( resolvedValue ){
		let data = resolvedValue["data"] as QLListData;
		assert( data.profile.view );
		this.selections.set( new SelectionModel<any>(data.profile.view.showSelector, []) );
		this.view.set( data.profile.view );
		const collectionName = data.schema.collectionName;
		this.resolvedData.set( data );
		//if( !this.profile )
			//this.profile = await this.profileStore.load(collectionName, QLList.defaultProfile );

		this.data.set( data.results[collectionName] );
		this.sideNav.set( data.routing );
		let paths = [];
		for( let x = this.route; x.routeConfig?.data && x.routeConfig?.data["name"]; x = x.parent )
			paths.push( x.routeConfig.data['name'] );
		this.componentPageTitle.title = paths[0];//.join( " | " ); 	//this.componentPageTitle.title ? `${this.componentPageTitle.title} | ${title}` : title;

/*		const order = ["name", "created", "updated", "deleted", "target", "description"];
		this.displayedFields = Field.filterSort( this.schema().fields, order, [...this.excludedColumns(), "description"], this.showDeleted() );
		if( !this.excludedColumns().find(x=>x=="description") )
			this.displayedFields.push( this.schema().fields.find(x=>x.name=="description") );
*/
		this.isLoading.set( false );
	};
	onSortChange( sort:Sort ){
		//let updateView = this.view().isUser && this.view().sort.length<2;
		let newView = new View( this.view() );
		newView.sort = [sort, ...newView.sort.filter(s=>s.active!=sort.active)];
		this.onViewShow( newView );
	}

	edit(){
		if( this.selection().deleted )
			this.ql.mutate( `restore${this.type}("id":${this.selection().id})`, (m)=>console.log(m) ).then( ()=>this.selection().deleted=null ).catch( (e)=>console.log(e) );
		else{
			try{
				this.router.navigate([this.selection().target], {relativeTo: this.route} );
			}catch( e ){
				this.snackbar.exceptionInfo( e, "Could not navigate to properties", (m)=>console.log(m) );
			}
		}
	}

	insert(){
		this.router.navigate( ['$new'], {relativeTo: this.route} );
	}

	async delete(){
		const purge = this.selection().deleted!=null;
		const type = purge ? "purge" : "delete";
		try{
			await this.ql.mutate(`${type}${this.type()}(id:${this.selection().id})`, (m)=>console.log(m) );
			if( !purge && this.showDeleted() )
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
	async onViewSave(view:View){
		this.data.set( [] );
		let profile = this.resolvedData().profile;
		if( view.isSystem && !profile.views.find(v=>v.name==view.name && v.isSystem) )
			view.type = ViewType.User;
		profile.upsertView( view, this.collectionName(), this.profileStore );
		if( view.type==ViewType.User )
			this.profileStore.save( `qlList/${this.collectionName()}/views`, profile.views.filter(v=>v.isUser).map(v=>v.toJson(this.tableSettings())) );

		let reload = await QLListResolver.load( this.ql, this.resolvedData(), this.routeStore );
		this.init( {data:reload} );
		this.isSettings.set( false );
	}
	async onViewShow(view:View){
		this.data.set( [] );
		this.isSettings.set( false );
		if( view.name?.endsWith("*") && view.isAdhoc )
		 	view.name = view.name.substring( 0, view.name.length-1 );
		view.type = ViewType.Adhoc;
		let profile = new PageProfile( this.resolvedData().profile );
		profile.upsertView( view, this.collectionName(), this.profileStore );
		this.refresh( profile );
	}
	async onChangeView(index:number){
		let profile = new PageProfile( this.resolvedData().profile );
		profile.currentViewIndex = index;
		ProfileStore.setViewIndex( this.collectionName(), index );
		this.refresh( profile );
	}
	async refresh( profile: PageProfile ){
		this.resolvedData().profile = profile;
		this.resolvedData().schema = new TableSchema( this.resolvedData().schema ); //TODO just copy
		const reload = await QLListResolver.load( this.ql, this.resolvedData(), this.routeStore );
		this.init( {data:reload} );
	}
	async onViewDelete(view:View){
		let profile = this.resolvedData().profile;
		profile.removeView( view.name, this.collectionName(), this.profileStore );
		profile.currentViewIndex = 0;
		let reload = await QLListResolver.load( this.ql, this.resolvedData(), this.routeStore );
		this.init( {data:reload} );
		this.isSettings.set( false );
	}

	async onToggleShowDeleted(){
		const showDeleted = !this.showDeleted();
		let view = new View( this.view() );
		view.setDeletedDisplayed( showDeleted );
		ProfileStore.setShowDeleted( this.collectionName(), showDeleted );
		let profile = new PageProfile( this.resolvedData().profile );
		profile.updateView( view );
		profile.showDeleted = showDeleted;
		this.refresh( profile );
	}

	colSuggestions():Record<string,any[]>{
		let suggestions: Record<string, any[]> = {};
		for( let field of this.view().fields ){
			let values = [];
			if( field.qlField.isNullable ){
				values.push("<null>");
				values.push("<not null>");
			}
			suggestions[field.name] = values;
		}
		for( let row of this.data() ){
			for( let col of Object.keys(row).filter(c=>row[c] && !suggestions[c].includes(row[c])) )
				suggestions[col].push( row[col] );
		}
		for( let col of Object.keys(suggestions) )
			suggestions[col] = suggestions[col].filter( (v,i,a) => a.indexOf(v)===i ).slice(0,100).sort((a, b)=>a-b);
		return suggestions;
	}

	onViewCancel(){
		this.isSettings.set( false );
	}

	sideNav = model.required<DocItem>();
	collectionDisplay = input.required<string>();

	isLoading = signal<boolean>( true );
	isSettings = signal<boolean>( false );
	selections = signal<SelectionModel<any>>(null);

	displayedFields = computed<ViewField[]>( ()=>{
		return this.view().fields.filter( v=>v.displayed );
	});
	@ViewChild('mainTable',{static: false}) _table:MatTable<any>;
	canPurge = computed<boolean>( ()=>this.tableSettings().canPurge );
	collectionName = computed<string>( ()=>this.schema().collectionName );
	columns():Record<string,string>{ return this.resolvedData().columns; }
	data = signal<any[]>([]);
	excludedColumns = computed<string[]>( ()=>this.tableSettings().excludedColumns );
	get name():string{ return <string>this.routeConfig.title; }
	enums = computed<Map<string, EnumValue[]>>( ()=>this.schema().enums );
	resolvedData = signal<QLListData>(null);
	get routeConfig(){ return this.route.routeConfig; }
	routeStore = inject( RouteStore );
	schema = computed<TableSchema>( ()=>this.resolvedData().schema );
	get sort():Sort{ return this.view().sort.length ? this.view().sort[0] : {active: "name", direction: "asc"}; }
	showDeleted = computed<boolean>( ()=>this.resolvedData().profile.showDeleted );
	showAdd = computed<boolean>( ()=>this.resolvedData().pageSettings.showAdd ?? true );
	tableSettings = computed<TableSettings>( ()=>this.resolvedData().routing.tableSettings );
	type = computed<string>( ()=>MetaObject.toTypeFromCollection(this.collectionName()) );
	view = signal<View>( null );
	profileStore = inject(ProfileStore);
}
