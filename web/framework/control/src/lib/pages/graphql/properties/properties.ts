import { Component, effect, input, output, AfterViewInit, EventEmitter, ViewChild, ViewChildren, ElementRef, OnInit, OnDestroy, QueryList, ChangeDetectorRef, computed, viewChildren, model, signal, CUSTOM_ELEMENTS_SCHEMA, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatFormFieldModule, MatLabel } from '@angular/material/form-field';
import { ActivatedRoute, Router } from '@angular/router';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { ComponentPageTitle } from 'jde-spa';
import { EnumValue, IGRAPHQL, IGraphQL } from '../../../services/graphql';
import { TableSchema } from '../../../model/ql/schema/table-schema';
import { Field, FieldKind } from '../../../model/ql/schema/field';
import { StringUtils } from '../../../utils/string-utils';
import { MatSelectModule } from '@angular/material/select';
import { MatInputModule } from '@angular/material/input';
import { MatChipGrid, MatChipsModule } from '@angular/material/chips';
import { MatButtonModule } from '@angular/material/button';

@Component({
    selector: 'properties',
    templateUrl: 'properties.html',
    imports: [CommonModule, MatButtonModule, MatChipsModule, MatChipGrid, MatInputModule, MatFormFieldModule, MatLabel, MatSelectModule],
		schemas: [CUSTOM_ELEMENTS_SCHEMA]
})
export class Properties implements OnInit{
	private route:ActivatedRoute = inject( ActivatedRoute );
	private router:Router = inject( Router );
	private componentPageTitle:ComponentPageTitle = inject( ComponentPageTitle );
	private cdr:ChangeDetectorRef = inject( ChangeDetectorRef );
	private cnsl:SnackbarService = inject( SnackbarService );
	private graphQL:IGraphQL = inject( IGRAPHQL );
	constructor(){
		effect( ()=>{
			this.componentPageTitle.detail = this.record()["name"] ?? `New ${this.schema().type}`;
		});
	}

	async ngOnInit(){
		this.isLoading.set( false );
	}

	onChange( field:string, value:string ){
		let f = this.ctor();
		let newRecord = new f( this.record() );
		newRecord[field] = value;
		this.record.set( newRecord );
	}

	originalOrder = ()=>0;//keyvalue pipe comparator: keep insertion order

	fields = computed<PropertyField[]>( ()=>{
		let y = [];
		let filter = (field: Field)=>
			[FieldKind.OBJECT,FieldKind.LIST,FieldKind.LIST].indexOf(field.type.underlyingKind)==-1
			&& !field.isBoolean
			&& !Properties.noShowFields.includes(field.name)
			&& !this.excludedColumns().includes(field.name);
		for( const field of this.schema().fields.filter(filter) ){
			let values = field.isEnum ? this.schema().enums.get(field.type.name) : undefined;
			y.push( new PropertyField(field, values) );
		}
		const order = ["target", "name"];
		const sort = ( a:PropertyField,b:PropertyField )=>{
			const bIndex = order.indexOf( b.name )+1;
			const aIndex = order.indexOf( a.name )+1;
			if( aIndex || bIndex )
				return ( aIndex || order.length )-( bIndex || order.length );
			else
				return a.name.localeCompare( b.name );
		}
		return y.sort( sort );
	});
	boolFields = computed<PropertyField[]>( ()=>{
		return [];
		//return this.fields().filter( (x)=>x.type==InputTypes.Bool );
	});
	getEnumId( field:PropertyField ):number{
		const value = this.record()[field.name];
		return value==undefined ? 0 : typeof value=="number" ? value : field.options!.find( (x)=>x.name==value )?.id ?? 0;
	}

	ctor = input.required<new (item: any) => any>();
	excludedColumns = input<string[]>([]);
	record = model.required<any>();
	schema = input.required<TableSchema>();
	type = input.required<string>();

	stringFields = viewChildren<ElementRef>( "stringField" );

	isLoading = signal<boolean>( true );

	get InputTypes(){ return InputTypes; }
	static noShowFields = ["id", "created", "attributes", "updated", "deleted"];
}
enum InputTypes{
	Select=-1,
	None=0,
	Text=1,
	Bool=2
}
class PropertyField{
	constructor( private field:Field, public options?:Array<EnumValue> )
	{}
	get name(){ return this.field.name; }
	get displayName(){ return this.field.name=="target" ? "Id" : StringUtils.idToDisplay( this.field.name ); }
	get nullable(){ return this.field.type.kind!=FieldKind.NON_NULL; }
	get type():InputTypes{
		let type = InputTypes.None;
		if( this.options )
			type = InputTypes.Select;
		else if( this.name=="description" )
			type = InputTypes.Text;
		else if( this.name.startsWith("is") )
			type = InputTypes.Bool;
		return type;
	}
}