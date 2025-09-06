import { filter } from 'rxjs/operators';
import { Component, AfterViewInit, OnInit, OnDestroy, Inject, ViewChild, Input, AfterContentInit, ContentChildren, QueryList, ContentChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, NavigationEnd, Params, Router, RouterModule, Routes} from '@angular/router';
import { MatDialog } from '@angular/material/dialog';
import { MatSortModule, Sort } from '@angular/material/sort';
import { MatTable, MatTableModule } from '@angular/material/table';
import { ComponentPageTitle } from 'jde-spa';

import {IErrorService} from '../../../services/error/IErrorService';
import {IGraphQL, TableSchema, Field, FieldKind}  from '../../../services/IGraphQL';
import {IProfile} from '../../../services/profile/IProfile';
import {Settings} from '../../../utilities/settings';
import {StringUtils} from '../../../utilities/StringUtils'
//import { SelectDialog } from '../select-dialog/select-dialog';
import { MetaObject } from '../../../utilities/JsonUtils';
import { MatToolbar } from '@angular/material/toolbar';
import { MatIcon } from '@angular/material/icon';

@Component( {
    selector: 'graph-ql-links',
    templateUrl: 'links.html',
    imports: [CommonModule, MatIcon, MatTable, MatTableModule, MatSortModule, MatToolbar]
})
export class GraphQLLinkComponent implements OnDestroy, OnInit, AfterViewInit{
	constructor( private route: ActivatedRoute, private router:Router, private dialog : MatDialog, private componentPageTitle:ComponentPageTitle, @Inject('IGraphQL') private graphQL: IGraphQL, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private cnsle: IErrorService )
	{}

	ngOnDestroy(){ this.profile.save(); }
	ngOnInit(){// @ts-ignore
		this.router.events.pipe( filter(e=>e instanceof NavigationEnd) ).subscribe( this.onNavigationEnd );
	}
	async ngAfterViewInit(){
		let self = this;
		await this.profile.loadedPromise;
		try{
			let mutations = await this.graphQL.mutations();
			const mutationA = `add${this.parentType}${this.schema.typeName}`;
			const mutationB = `add${this.schema.typeName}${this.parentType}`;
			const mutationC = `add${this.schema.typeName}`
			var mutation = mutations.find( (x)=>x.name==mutationA || x.name==mutationB || x.name==mutationC ); if( !mutation ) throw `could not find mutation ${mutationA}/${mutationB}`;
			this.mutation = mutation.name.substring( 3 );
			console.log( `objectCollectionName=${this.schema.objectCollectionName}` );
			console.log( `parent=${JSON.stringify(this.parent)}` );
			// for( var i of this.items )
			// 	console.log( i );
			// for( var f of this.schema.fields )
			// 	console.log( f );
		}
		catch( e ){
			this.cnsle.error( "could not load page.", e );
		}
		this.viewPromise = Promise.resolve( true );
	}
	onNavigationEnd =( val:NavigationEnd )=>{}
/*	edit( fieldName:string, element ){
		var field = this.schema.fields.find( (x)=>x.name==fieldName );
		this.graphQL.schema( [field.type.underlyingName] ).then( (x)=>{
			const dialogRef = this.dialog.open( SelectDialog,{
				width: '600px',
				height: '650px',
				data: { schema: x[0], selectedIds:element[fieldName], query:fieldName, mutation:this.mutation, linkTo:this.parent.id, linkToField: this.parentTypeField, subTo:element.id ?? `"${element[ element.target ? "target" : "name" ]}"`, subToField: element.id ? this.schema.subType.idReferenceName : element.target ? "target" : "name", title:StringUtils.capitalize(fieldName), includeDeleted: true, graphQL:this.graphQL }
			});
			dialogRef.afterClosed().subscribe( result =>{
				if( result )
					this.load();
			});
		}).catch( (e)=>console.error(e) );
	}*/
	async load(){
		this.viewPromise = null;
		const valueMember = this.schema.objectCollectionName;
		let ql = `${this.parentSelect}(filter:{ id:{eq:${this.parent.id}}}){ ${valueMember}{${this.schema.columns}} }`;
		let data = await this.graphQL.query( ql );
		this.parent[valueMember].length=0;
		data[this.parentSelect][valueMember].forEach( x => {this.parent[valueMember].push(x);} );
		this.viewPromise = Promise.resolve( true );
	}
	cellClick( row:any ){  this.selection = this.selection == row ? null : row; }
	addLink(){
		let show = (schema:TableSchema)=>{
			let selectedIds = this.items.map( (x)=>x.id );
			const dialogRef = this.dialog.open( SelectDialog,{
				width: '600px',
				height: '650px',
				data: { selectedIds:selectedIds, schema:schema, mutation:this.mutation, linkTo:this.parent.id, linkToField: this.parentTypeField, title:`${this.parent.name} ${this.schema.display}` }
			});
			dialogRef.afterClosed().subscribe( result =>{
				if( result )
					this.load();
			});
		};
		if( this.schema.subType ){
			this.graphQL.schema( [this.schema.subType.typeName] ).then( (x)=>{
				show( x[0] );
			}).catch( (e)=>console.error(e) );
		}
		else
			show( this.schema );
	}
	removeLink(){
		let ql = `{remove${this.mutation}(${this.parentTypeField}: ${this.parent.id}, `;
		if( this.selection.id )
			ql+=`"${new MetaObject(this.schema.typeName).singular}Id": ${this.selection.id}} ) }`;
		else if( this.selection.target )
			ql+=`"target": "${this.selection.target}" })}`;
		else
			ql+=`"name": "${this.selection.name}" })}`;

		this.graphQL.mutation( ql ).then( (x)=>{
			this.selection = null;
			this.load();
		} ).catch( (e)=>{this.cnsle.error(e);} );
	}
	sortData(event)
	{}

	get haveSelection():boolean{ return !!this.selection; }
	selection:any|null|undefined;
	@Input() schema:TableSchema;
	@ViewChild(MatTable, {static: true}) table: MatTable<any>;
	get items(){ return this.parent[this.schema.objectCollectionName]; }
	@Input() parent:any;
	get id(){ return  this.router.url.substring( 0, this.router.url.lastIndexOf('/')+1 )+this.schema.objectReferenceName; }

	get profile():Settings<UserSettings>{ return this.#profile ?? ( this.#profile = new Settings<UserSettings>(UserSettings, this.id, this.profileService) ); } #profile:Settings<UserSettings>;
	get sort(){ return this.profile.value.sort; }
	get displayedColumns():Field[]{ return this.schema.fields.filter( (x)=>x.displayed && !["created", "updated", "deleted", "target"].includes(x.name) ); }
	get displayedColumnNames(){ return this.displayedColumns.map( (x)=>x.name ); };// = ["name","target","description","authenticator", "deleted"];
	get stringColumnNames(){ return this.enumColumnNames.concat( this.displayedColumns.filter((x)=>x.type.underlyingKind==FieldKind.SCALAR && x.type.underlyingName=="String" ).map( (x)=>x.name) ); }
	get enumColumnNames(){ return this.displayedColumns.filter( (x)=>x.type.underlyingKind==FieldKind.ENUM ).map( (x)=>x.name ); }
	get objectColumnNames(){ return this.displayedColumns.filter( (x)=>x.type.underlyingKind==FieldKind.OBJECT ).map( (x)=>x.name ); }
	get listColumnNames(){ return this.displayedColumns.filter( (x)=>x.type.underlyingKind==FieldKind.LIST ).map( (x)=>x.name ); }
	get dateColumnNames(){ return this.displayedColumns.filter( (x)=>x.type.underlyingName=="DateTime" ).map( (x)=>x.name ); }
	mutation:string;
	@Input() parentType:string;
	get parentSelect(){ return this.parentType.charAt(0).toLowerCase() + this.parentType.slice(1) }
	get parentTypeField(){ return this.parentSelect+"Id"; }
	viewPromise:Promise<boolean>;
	showDeleted=false;
}

class UserSettings{
	assign( value:UserSettings ){   }
	sort:Sort = {active: "name", direction: "asc"};
}