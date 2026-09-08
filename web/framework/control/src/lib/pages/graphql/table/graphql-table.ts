import { Component, input, effect, model, Signal, output, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { SelectionModel } from '@angular/cdk/collections';
import { MatTableModule } from '@angular/material/table';
import { MatSortModule, Sort } from '@angular/material/sort';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import {FieldKind} from '../../../model/ql/schema/field';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatChipsModule } from '@angular/material/chips';
import { MatIcon } from '@angular/material/icon';
import { StringUtils } from '../../../utils/string-utils';
import { ViewField } from '../../../model/ql/view';
import { QLRow } from '../../../model/ql/target-row';

@Component({
	selector: 'ql-table',
	styleUrls: ['./graphql-table.scss'],
	templateUrl: './graphql-table.html',
	imports: [CommonModule, MatCheckbox, MatChipsModule, MatIcon, MatTableModule, MatSortModule]
})
export class GraphQLTable{
	private cnsle:SnackbarService = inject( SnackbarService );

	checkboxLabel( row?: QLRow ): string{
		return row
			? `${this.selections().isSelected(row) ? 'deselect' : 'select'} row ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
	}
	toggle( row: QLRow ){//rows, not ids - the row click path, the highlight and ql-list's selection() all key off the row object
		const newSelections = this.selections().isSelected(row) ? this.selections().selected.filter( (x)=>x!=row ) : this.selections().selected.concat( row );
		this.selections.set( new SelectionModel<QLRow>(this.selections().isMultipleSelection(), newSelections) );
	}

	toggleAll(){
		if( this.isAllSelected() )
			this.selections.set( new SelectionModel<QLRow>(this.selections().isMultipleSelection(), []) );
		else
			this.selections.set( new SelectionModel<QLRow>(this.selections().isMultipleSelection(), [...this.dataSource()()]) );
	}

	cellClick( row:QLRow ){
		const isSelected = this.selections().isSelected( row );
		const multi = this.selections().isMultipleSelection();
		let selections = [];
		if( multi )
			selections = isSelected ? this.selections().selected.filter( (x)=>x!=row ) : this.selections().selected.concat( row );
		else
			selections = isSelected ? [] : [row];
		this.selections.set( new SelectionModel<QLRow>(multi, selections) );
		this.onRowActivate.emit( row );
	}

	edit( column:string, element: QLRow ): void{
	}

	delete( row?: QLRow ): string{
		return row
			? `${this.selections().isSelected(row) ? 'deselect' : 'select'} row ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
	}

	isAllSelected(){ return this.selections().selected.length==this.dataSource()().length; }
	isSelected( row:QLRow ){ return this.selections().isSelected(row); }
	columnName( colName:string ){ return this.displayedFields().find(f=>f.name===colName)?.displayName ?? StringUtils.capitalize(colName); }
	//Applied to the header cell and the data cell from the one setting, so the two cannot drift apart.  `align` is expanded
	//rather than passed through: it is not a CSS property, and a mat-cell is a flex container - text-align on its own leaves
	//the content wherever justify-content put it.
	columnStyle( colName:string ):Record<string,string>{
		const style = this.displayedFields().find( f=>f.name===colName )?.style;
		if( !style )
			return {};
		const y:Record<string,string> = {};
		for( const [key,value] of Object.entries(style) ){
			if( value==null )
				continue;
			if( key=="align" ){
				y["justifyContent"] = value=="right" ? "flex-end" : value=="center" ? "center" : "flex-start";
				y["textAlign"] = `${value}`;
			}
			else
				y[key] = `${value}`;
		}
		return y;
	}
	objectValue( obj: unknown ): string{
		if( obj==null )
			return '';
		if( typeof obj === 'string' )
			return obj;
		if( typeof obj!=='object' )
			return '';//as before: a scalar has no values to show, and the named/one-value rules below are for objects
		const named = (obj as {name?:unknown}).name;
		if( named!=undefined )
			return `${named}`;
		const values = Object.values( obj ).filter( v=>v!=null );//no `name` - a single remaining property is the display value, e.g. opcSessions{count}.
		return values.length==1 ? `${values[0]}` : '';
	}
	sortable( colName:string ){ return this.displayedFields().find(f=>f.name===colName)?.type.underlyingKind!=FieldKind.OBJECT; }//orderBy on a grafted object field is a server error.

	//The tone class for a chip cell.  The column names the tones, the stylesheet owns the colours, so a value the column did
	//not list simply gets the theme's default chip rather than the wrong colour.
	chipClass( colName:string, row:QLRow ):string{
		const tones = this.displayedFields().find( f=>f.name===colName )?.chip;
		const tone = tones?.[ this.objectValue(row[colName]) ];
		return tone ? `chip-${tone}` : "";
	}

	dataSource=input.required<Signal<QLRow[]>>();
	displayedFields = input.required<ViewField[]>();
	selections=model.required<SelectionModel<QLRow>>();
	//showDeleted = input<boolean>( false );
	sort = model<Sort>();
	onSortChange = output<Sort>();
	onRowActivate = output<QLRow>();

	get displayedColumnNames(){ return (this.selections().isMultipleSelection() ? ["select"] : []).concat( this.displayedFields().filter((x)=>x.displayed).map((x)=>x.name) ); };
	//A chip column is whatever kind it always was - it just renders as a chip, so it has to leave the bucket it would
	//otherwise fall in or the same matColumnDef is declared twice and the table throws.
	get chipColumnNames(){ return this.displayedFields().filter( (x)=>x.chip ).map( (x)=>x.name ); }
	get stringColumnNames(){ return this.displayedFields().filter( (x)=>!x.chip && ((x.type.underlyingKind==FieldKind.SCALAR && x.type.underlyingName=="String") || x.type.underlyingKind==FieldKind.ENUM) ).map( (x)=>x.name ); }
	get objectColumnNames(){ return this.displayedFields().filter( (x)=>!x.chip && x.type.underlyingKind==FieldKind.OBJECT ).map( (x)=>x.name ); }
	get listColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.LIST ).map( (x)=>x.name ); }
	get dateColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="DateTime" ).map( (x)=>x.name ); }
	get boolColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="Boolean" ).map( (x)=>x.name ); }
	get uintColumnNames(){ return this.displayedFields().filter( (x)=>["UInt", "ID"].includes(x.type.underlyingName) ).map( (x)=>x.name ); }
}