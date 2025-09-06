import { Component, Inject, input, effect, model, Signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { SelectionModel } from '@angular/cdk/collections';
import { MatTableModule } from '@angular/material/table';
import { MatSortModule, Sort } from '@angular/material/sort';
import { IErrorService } from '../../../services/error/IErrorService';
import {Field, FieldKind} from '../../../model/ql/schema/Field';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatIcon } from '@angular/material/icon';
import { StringUtils } from '../../../utilities/StringUtils';

@Component({
    selector: 'ql-table',
    styleUrls: ['table.scss'],
    templateUrl: 'table.html',
    imports: [CommonModule, MatCheckbox, MatIcon, MatTableModule, MatSortModule]
})
export class GraphQLTable{
	constructor( @Inject('IErrorService') private cnsle: IErrorService ){
		effect(() => {
			let field = this.displayedFields().find( (c)=>c.name=="deleted" );
			if( field )
				field.displayed = this.showDeleted();
		});
	}

	checkboxLabel( row?: any ): string{
		return row
			? `${this.selections().isSelected(row) ? 'deselect' : 'select'} row ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
	}
	toggle( id ){
		const newSelections = this.selections().isSelected(id) ? this.selections().selected.filter( (x)=>x!=id ) : this.selections().selected.concat( id );
		console.log( `table set selections: ${newSelections}` );
		this.selections.set( new SelectionModel<number>(this.selections().isMultipleSelection(), newSelections) );
	}

	toggleAll(){
		if( this.isAllSelected() )
			this.selections.set( new SelectionModel<number>(this.selections().isMultipleSelection(), []) );
		else
			this.selections.set( new SelectionModel<number>(this.selections().isMultipleSelection(), [...this.dataSource()().map(x=>x.id)]) );
	}

	cellClick( row:any ){
		const isSelected = this.selections().isSelected( row );
		const multi = this.selections().isMultipleSelection();
		let selections = [];
		if( multi )
			selections = isSelected ? this.selections().selected.filter( (x)=>x!=row ) : this.selections().selected.concat( row );
		else
			selections = isSelected ? [] : [row];
		this.selections.set( new SelectionModel<any>(multi, selections) );
	}

	edit( column:string, element: any ): void{
	}

	delete( row?: any ): string{
		return row
			? `${this.selections().isSelected(row) ? 'deselect' : 'select'} row ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
	}

	isAllSelected(){ return this.selections().selected.length==this.dataSource()().length; }
	isSelected( row:any ){ return this.selections().isSelected(row); }
	sortData( event )
	{}
	columnName( fieldName ){ return StringUtils.capitalize(fieldName); }

	dataSource=input.required<Signal<any[]>>();
	displayedFields = input.required<Field[]>();
	selections=model.required<SelectionModel<number>>();
	showDeleted = input<boolean>( false );
	sort = input<Sort>( {active:"name", direction: 'asc'} );

	get displayedColumnNames(){ return (this.selections().isMultipleSelection() ? ["select"] : []).concat( this.displayedFields().filter((x)=>x.displayed).map((x)=>x.name) ); };
	get stringColumnNames(){ return this.displayedFields().filter( (x)=>(x.type.underlyingKind==FieldKind.SCALAR && x.type.underlyingName=="String") || x.type.underlyingKind==FieldKind.ENUM ).map( (x)=>x.name ); }
	get objectColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.OBJECT ).map( (x)=>x.name ); }
	get listColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.LIST ).map( (x)=>x.name ); }
	get dateColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="DateTime" ).map( (x)=>x.name ); }
	get boolColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="Boolean" ).map( (x)=>x.name ); }
	get uintColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="UInt" ).map( (x)=>x.name ); }
}