import { Component, input, effect, model, Signal, output, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { SelectionModel } from '@angular/cdk/collections';
import { MatTableModule } from '@angular/material/table';
import { MatSortModule, Sort } from '@angular/material/sort';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import {FieldKind} from '../../../model/ql/schema/field';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatIcon } from '@angular/material/icon';
import { StringUtils } from '../../../utils/string-utils';
import { ViewField } from '../../../model/ql/view';

@Component({
	selector: 'ql-table',
	styleUrls: ['./graphql-table.scss'],
	templateUrl: './graphql-table.html',
	imports: [CommonModule, MatCheckbox, MatIcon, MatTableModule, MatSortModule]
})
export class GraphQLTable{
	private cnsle:SnackbarService = inject( SnackbarService );

	checkboxLabel( row?: any ): string{
		return row
			? `${this.selections().isSelected(row) ? 'deselect' : 'select'} row ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
	}
	toggle( row: any ){//rows, not ids - the row click path, the highlight and ql-list's selection() all key off the row object
		const newSelections = this.selections().isSelected(row) ? this.selections().selected.filter( (x)=>x!=row ) : this.selections().selected.concat( row );
		this.selections.set( new SelectionModel<any>(this.selections().isMultipleSelection(), newSelections) );
	}

	toggleAll(){
		if( this.isAllSelected() )
			this.selections.set( new SelectionModel<any>(this.selections().isMultipleSelection(), []) );
		else
			this.selections.set( new SelectionModel<any>(this.selections().isMultipleSelection(), [...this.dataSource()()]) );
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
		this.onRowActivate.emit( row );
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
	columnName( colName:string ){ return this.displayedFields().find(f=>f.name===colName)?.displayName ?? StringUtils.capitalize(colName); }
	columnStyle( colName:string ):Record<string,string>{
		const style = this.displayedFields().find( f=>f.name===colName )?.style;
		return style ? Object.fromEntries( Object.entries(style).filter(([,v])=>v!=null).map(([k,v])=>[k,`${v}`]) ) : {};
	}
	objectValue( obj: any ): string{
		if( obj==null || typeof obj === 'string' )
			return obj ?? '';
		if( obj.name!=undefined )
			return obj.name;
		const values = Object.values( obj ).filter( v=>v!=null );//no `name` - a single remaining property is the display value, e.g. opcSessions{count}.
		return values.length==1 ? `${values[0]}` : '';
	}
	sortable( colName:string ){ return this.displayedFields().find(f=>f.name===colName)?.type.underlyingKind!=FieldKind.OBJECT; }//orderBy on a grafted object field is a server error.

	dataSource=input.required<Signal<any[]>>();
	displayedFields = input.required<ViewField[]>();
	selections=model.required<SelectionModel<any>>();
	//showDeleted = input<boolean>( false );
	sort = model<Sort>();
	onSortChange = output<Sort>();
	onRowActivate = output<any>();

	get displayedColumnNames(){ return (this.selections().isMultipleSelection() ? ["select"] : []).concat( this.displayedFields().filter((x)=>x.displayed).map((x)=>x.name) ); };
	get stringColumnNames(){ return this.displayedFields().filter( (x)=>(x.type.underlyingKind==FieldKind.SCALAR && x.type.underlyingName=="String") || x.type.underlyingKind==FieldKind.ENUM ).map( (x)=>x.name ); }
	get objectColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.OBJECT ).map( (x)=>x.name ); }
	get listColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.LIST ).map( (x)=>x.name ); }
	get dateColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="DateTime" ).map( (x)=>x.name ); }
	get boolColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="Boolean" ).map( (x)=>x.name ); }
	get uintColumnNames(){ return this.displayedFields().filter( (x)=>["UInt", "ID"].includes(x.type.underlyingName) ).map( (x)=>x.name ); }
}