import { ProfileStore } from "jde-spa";
import { View, ViewFieldSettings } from "../../../model/ql/View";
import { assert, TableSchema } from "jde-framework";

export class PageProfile{
	constructor( args?:PageProfile ){
		if( args ){
			this.showDeleted = args.showDeleted;
			this.currentViewIndex = args.currentViewIndex;
			this.views = args.views;
		}
	}
	upsertView( view:View, collectionName:string, profileStore:ProfileStore ){
		let existing = this.views.findIndex( v=>v.name==view.name && view.type==v.type );
		if( existing>=0 ){
			this.views[existing] = new View( view );
			this.currentViewIndex = existing;
		}
		else{
			this.views.push( new View(view) );
			this.currentViewIndex = this.views.length - 1;
		}
	}
	async loadViews( collectionName:string, profileStore:ProfileStore, schema:TableSchema ){
		const views = await profileStore.loadClassArray<View>( `qlList/${collectionName}/views`, View, schema );
		this.views.push( ...views );
	}
	async removeView( viewName:string, collectionName:string, profileStore:ProfileStore ){
		this.views = this.views.filter( v=>v.name!=viewName );
		await profileStore.save( `qlList/${collectionName}/views`, this.views.filter(v=>v.isUser) );
	}
	updateView( view:View ){
		this.views[this.views.findIndex( v=>v.name==view.name && view.type==v.type)] = view;
	}
	showDeleted:boolean;
	currentViewIndex:number=0;
	get view():View{ return this.views[this.currentViewIndex]; }
	views:View[] = [];
}
export class PageSettings{
	constructor( x:any ){
		this.configColumns = x.columns ? [ ...x.columns ] : [];
		this.excludedColumns = x.excludedColumns;
		this.name = x.name;
		this.showAdd = x.showAdd;
		this.canPurge = x.canPurge!==false;
		this.table = x.table ?? x.id;
	}
	canPurge:boolean;
	configColumns:(string|ViewFieldSettings)[];
	excludedColumns:string[];
	name:string;
	showAdd:boolean;
	table:string;
	get type():string{ return this.table ?? this.name[0].toLowerCase()+this.name.substring(1); }
}
