import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, effect, computed } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, Router} from '@angular/router';
import {Sort} from '@angular/material/sort';
import { MatTable } from '@angular/material/table';
import {IErrorService} from '../../../services/error/IErrorService'
import {IGraphQL } from '../../../services/IGraphQL';
import {Field} from '../../../model/ql/schema/Field';
import {TableSchema}  from '../../../model/ql/schema/TableSchema';
import {MetaObject}  from '../../../model/ql/schema/MetaObject';

import { ComponentPageTitle } from 'jde-spa';
import { GraphQLTable } from '../../GraphQL/table/table';
import { SelectionModel } from '@angular/cdk/collections';

@Component( {
		selector: 'ql-selector',//.main-content.mat-drawer-container.my-content
		styleUrls: ['ql-selector.scss'],
		templateUrl: './ql-selector.html',
		host: {class:'main-content mat-drawer-container my-content'},
		imports: [CommonModule, GraphQLTable]
})
export class QLSelector implements OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IErrorService') private snackbar: IErrorService )
	{}

	async ngOnInit(){
		try{
			const columns = ["id", "name", "description"];
			const input = this.excludedIds().length  ? `(id: {notIn: ${JSON.stringify(this.excludedIds())}})` : "";
			this.data.set( (await this.ql().query(`${this.collectionName()}${input}{${columns.join(" ")}}`))[this.collectionName()] );
			if( !this.schemaInput() )
				this.#schema = await this.ql().schemaWithEnums( this.collectionName() );
			this.displayedFields = Field.filterSort( this.schema.fields.filter((x)=>columns.includes(x.name)), columns, this.excludedColumnsInput() );
			this.isLoading.set( false );
		}
		catch( e ){
			this.snackbar.exceptionInfo( e, "Could not load values", (m)=>console.log(m) );
		}
	};
	sortData( options:Sort ){
		const values = [ ...this.data() ];
		const multiplier = options.direction === 'asc' ? 1 : -1;
		const name = options.active;
		this.data.set( values.sort((a, b) =>{
			let lessThan = a[name]<b[name];
			return (lessThan ? -1 : 1)*multiplier;
		}) );
		this._table.renderRows();
	}

	type = input.required<string>();

	selections = model.required<SelectionModel<number>>();
	isLoading = signal<boolean>( true );
	collectionName = computed<string>( ()=> MetaObject.toCollectionName(this.type()) );
	displayedFields:Field[];
	excludedIds = input<number[]>( [] );
	@ViewChild('mainTable',{static: false}) _table:MatTable<any>;
	data=signal<any[]>( null );
	excludedColumnsInput = input<string[]>([]);
	get name():string{ return <string>this.routeConfig.title; }
	get routeConfig(){ return this.route.routeConfig;}
	schemaInput=input<TableSchema>();
	get schema():TableSchema{ return this.schemaInput() ?? this.#schema; } #schema:TableSchema;
	ql=input.required<IGraphQL>();

	get sort():Sort{ return {active: "name", direction: "asc"}; }
}
