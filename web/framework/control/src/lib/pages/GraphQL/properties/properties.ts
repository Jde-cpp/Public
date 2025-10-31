import {Component, effect, Inject, input, output, AfterViewInit, EventEmitter, ViewChild, ViewChildren, ElementRef, OnInit, OnDestroy, QueryList, ChangeDetectorRef, computed, viewChildren, model, signal, CUSTOM_ELEMENTS_SCHEMA} from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatFormFieldModule, MatLabel } from '@angular/material/form-field';
import { ActivatedRoute, Router } from '@angular/router';
import { IErrorService } from '../../../services/error/IErrorService';
import { ComponentPageTitle } from 'jde-spa';
import { EnumValue, IGraphQL } from '../../../services/IGraphQL';
import { TableSchema } from '../../../model/ql/schema/TableSchema';
import { Field, FieldKind } from '../../../model/ql/schema/Field';
import { StringUtils } from '../../../utils/StringUtils';
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
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IGraphQL') private graphQL: IGraphQL, private cdr: ChangeDetectorRef, @Inject('IErrorService') private cnsl: IErrorService ){
		effect( ()=>{
			this.componentPageTitle.detail = this.record()["name"] ?? `New ${this.schema().type}`;
		});
	}

	async ngOnInit(){
		const order = ["target", "name"];
		const sort = ( x:PropertyField,y:PropertyField )=>{
			const yIndex = order.indexOf( y.name )+1;
			const xIndex = order.indexOf( x.name )+1;
			if( xIndex || yIndex )
				return ( xIndex || order.length )-( yIndex || order.length );
			else
				return x.name.localeCompare( y.name );
		}
		this.fields().sort( sort );

		this.isLoading.set( false );
	}

	onChange( field:string, value:string ){
		let f = this.ctor();
		let newRecord = new f( this.record() );
		newRecord[field] = value;
		this.record.set( newRecord );
	}

	originalOrder = ( a, b )=> {return 0;}

	fields = computed<PropertyField[]>( ()=>{
		let y = [];
		let filter = (field)=>
			[FieldKind.OBJECT,FieldKind.LIST,FieldKind.LIST].indexOf(field.type.underlyingKind)==-1
			&& field.type.ofType?.name!='Boolean'
			&& Properties.noShowFields.indexOf(field.name)==-1
			&& this.excludedColumns().indexOf(field.name)==-1;
		for( const field of this.schema().fields.filter(filter) ){
			let values = field.type.underlyingKind==FieldKind.ENUM ? this.schema().enums.get(field.type.name) : undefined;
			y.push( new PropertyField(field, values) );
		}
		return y;
	});
	boolFields = computed<PropertyField[]>( ()=>{
		return [];
		//return this.fields().filter( (x)=>x.type==InputTypes.Bool );
	});
	getEnumId( field:PropertyField ):number{
		let stringValue = this.record()[field.name];
		return stringValue ? field.options.find( (x)=>x.name==stringValue ).id : 0;
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