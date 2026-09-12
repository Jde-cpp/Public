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
import { MatIconButton } from '@angular/material/button';
import { MatTooltip } from '@angular/material/tooltip';
import { StringUtils } from '../../../utils/string-utils';
import { ViewField } from '../../../model/ql/view';
import { QLRow } from '../../../model/ql/slug-row';

@Component({
	selector: 'ql-table',
	styleUrls: ['./graphql-table.scss'],
	templateUrl: './graphql-table.html',
	imports: [CommonModule, MatCheckbox, MatChipsModule, MatIcon, MatIconButton, MatTableModule, MatSortModule, MatTooltip]
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

	//A live-toggle cell (ViewFieldSettings.liveToggle).  Deliberately an icon button and not a mat-slide-toggle:  a slide
	//toggle keeps its own checked state, so a mutation that FAILS would leave the switch showing the state the row never
	//reached.  An icon renders straight off the row, so there is nothing to desync - the cell only ever shows what the
	//server confirmed.
	isLive( column:string, row:QLRow ):boolean{ return row[column]==null; }
	//The verb for the direction a click would take this row - what the tooltip, the aria-label and the confirmation all read.
	liveToggleLabel( column:string, row:QLRow ):string{
		const settings = this.displayedFields().find( f=>f.name===column )?.liveToggle;
		return (this.isLive(column,row) ? settings?.disable : settings?.enable) ?? "";
	}

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
	pendingLive = input<unknown[]>( [] );//row ids whose live-toggle mutation is in flight - the switch is disabled until the server answers
	onSortChange = output<Sort>();
	onRowActivate = output<QLRow>();
	onToggleLive = output<QLRow>();

	get displayedColumnNames(){ return (this.selections().isMultipleSelection() ? ["select"] : []).concat( this.displayedFields().filter((x)=>x.displayed).map((x)=>x.name) ); };
	//A chip column is whatever kind it always was - it just renders as a chip, so it has to leave the bucket it would
	//otherwise fall in or the same matColumnDef is declared twice and the table throws.
	get chipColumnNames(){ return this.displayedFields().filter( (x)=>x.chip ).map( (x)=>x.name ); }
	//As chipColumnNames: a switch column leaves the bucket it would otherwise fall in (`deleted` is a DateTime) or the same
	//matColumnDef is declared twice and mat-table throws.
	get liveToggleColumnNames(){ return this.displayedFields().filter( (x)=>x.liveToggle ).map( (x)=>x.name ); }
	get stringColumnNames(){ return this.displayedFields().filter( (x)=>!x.chip && ((x.type.underlyingKind==FieldKind.SCALAR && x.type.underlyingName=="String") || x.type.underlyingKind==FieldKind.ENUM) ).map( (x)=>x.name ); }
	get objectColumnNames(){ return this.displayedFields().filter( (x)=>!x.chip && x.type.underlyingKind==FieldKind.OBJECT ).map( (x)=>x.name ); }
	get listColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingKind==FieldKind.LIST ).map( (x)=>x.name ); }
	get dateColumnNames(){ return this.displayedFields().filter( (x)=>!x.liveToggle && x.type.underlyingName=="DateTime" ).map( (x)=>x.name ); }
	get boolColumnNames(){ return this.displayedFields().filter( (x)=>x.type.underlyingName=="Boolean" ).map( (x)=>x.name ); }
	get uintColumnNames(){ return this.displayedFields().filter( (x)=>["UInt", "ID"].includes(x.type.underlyingName) ).map( (x)=>x.name ); }
}