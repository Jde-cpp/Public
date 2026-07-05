import { ChangeDetectionStrategy, ChangeDetectorRef, Component, ElementRef, OnDestroy, OnInit, QueryList, Signal, ViewChild, ViewChildren, WritableSignal, input, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import {FormControl, FormsModule, ReactiveFormsModule} from '@angular/forms';
import { MatButtonModule } from "@angular/material/button";
import {type MatDatepickerInputEvent, MatDatepickerModule} from '@angular/material/datepicker';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatTable, MatTableModule } from '@angular/material/table';
import { Days, Filter, Operator, View, FieldFilter, ViewField } from '../../../../../model/ql/View';
import { StringUtils } from '../../../../../utils/StringUtils';
import {MatAutocompleteModule, type MatAutocompleteSelectedEvent} from '@angular/material/autocomplete';
import {MatCheckboxModule} from '@angular/material/checkbox';
import { MatChipsModule, type MatChipInputEvent } from "@angular/material/chips";
import {MatFormFieldModule} from '@angular/material/form-field';
import { MatInput } from "@angular/material/input";
import { verify, Field, TableSchema } from 'jde-framework';
import { BehaviorSubject, from, map, Observable, startWith, Subject } from 'rxjs';
import { COMMA, ENTER } from '@angular/cdk/keycodes';
import { provideNativeDateAdapter } from '@angular/material/core';

type ColumnFilter = {field:Field, filter: Filter, displayName:string};
@Component( {
		selector: 'ql-list-settings-filter',
		styleUrls: ['ql-list-settings-filter.scss'],
		templateUrl: './ql-list-settings-filter.html',
		host: {class:'main-content.mat-drawer-container.my-content'},
	  providers: [provideNativeDateAdapter()],
		imports: [CommonModule, FormsModule, MatAutocompleteModule, MatButtonModule, MatCheckboxModule, MatChipsModule, MatDatepickerModule, MatFormFieldModule, MatIconModule, MatSelectModule, MatTableModule, MatInputModule, ReactiveFormsModule],
		//changeDetection: ChangeDetectionStrategy.OnPush
})
export class QLListSettingsFilter implements OnInit{
	constructor( private cdr: ChangeDetectorRef ){}

	ngOnInit(){
		for( let fieldFilter of this.view().fieldFilters ){
			this.dataSource.push( { field: fieldFilter.field, filter: fieldFilter.filter, displayName: this.columns()[fieldFilter.field.name] } );
			this.addSignals( fieldFilter.field, fieldFilter.filter.operator );
		}
		this.dataSource.push( {field: new Field({}), filter: {operator: Operator.None, value: []}, displayName: ""} );
	}

	onOperatorChange( op: Operator, col: ColumnFilter ){
		//let arg = this.args.get(col.field.name);
		this.operatorSignals.get(col.field.name)!.set(op);
		col.filter.operator = op;
		//arg.filter.operator = op;
	}
	filter( col:ColumnFilter ):string[]{
		//let arg = this.args.get(col.field.name);
		return col.filter.value as string[];
	}
	onRemove(col:ColumnFilter, item: string){
		//let arg = this.args.get(col.field.name);
		col.filter.value.splice( col.filter.value.indexOf(item), 1 );
		this.autoCompleteSubjects.get(col.field.name)!.next(this.colSuggestions(col, ""));
	}
	onAddValue( value:any, col: ColumnFilter ){
		//let arg = this.args.get(col.field.name);
		col.filter.value.push( value );
		let index = 0;
		for( let input of this.autoCompleteInputs.keys() ){
			if( input == col.field.name )
				break;
			++index;
		}
		let fc = this.autoCompleteInputs.get(col.field.name)!;
		fc.reset();
		this.inputElements.toArray()[index].nativeElement.value = "";
	}
	onAddFilter( columnName: string ){
		verify( !this.dataSource.find( c=>c.field?.name == columnName )?.field );
		let field = this.schema().fields.find(f=>f.name == columnName)!;
		const index = this.dataSource.length-1;
		this.dataSource.push( this.dataSource[index] );
		let operator = field.isDateTime ? Operator.Greater : Operator.In;
		this.dataSource[index] = { field: field, filter: {operator: operator, value: []}, displayName: this.columns()[field.name] };
		this.addSignals( field, operator );
		this.table.renderRows();
	}
	addSignals( field: Field, operator: Operator ){
		this.operatorSignals.set( field.name, signal(operator) );
		if( field.isNullable )
			this.nullSignals.set( field.name, signal(NullCriteria.None) );
	}
	onChangeDate( event:MatDatepickerInputEvent<Date>, col: ColumnFilter ){
		//let arg = this.args.get( col.field.name );
		if( col.filter.operator==Operator.Greater )
			col.filter.value[0] = new Days(event.value!);
		else
			col.filter.value[0] = event.value;
	}
	dateValue( col: ColumnFilter ): Date|undefined{
		//let arg = this.args.get( col.field.name );
		if( !col.filter.value.length )
			return undefined;
		for( let val of col.filter.value ){
			if( val instanceof Days )
				return (val as Days).fromNow();
			else if( val instanceof Date )
				return val;
		}
		return undefined;
	}
	colSuggestions(col:ColumnFilter, value:string):string[]{
		let suggestions = this.suggestions()[col.field.name] as string[];
		if( !suggestions ) //filter column not shown.
			return [];
		//let arg = this.args.get( col.field.name );
		let result = suggestions.filter( s=>{
			const existing = col.filter.value;
			if( existing.includes(s) )
				return false;
			if( ["<null>", "<not null>"].includes(s) ){
				if( [Operator.Less, Operator.Greater].includes(col.filter.operator) )
					return false;
				else if( s=="<not null>" )
					return !existing.length;
			}
			return existing.indexOf( "<not null>" )==-1;
		});
		return result;
	}
	useObservable = false;
	autoCompleteValues(col:ColumnFilter): Observable<string[]>{
		if( !this.useObservable )
			return this.autoCompleteSubjects.get(col.field.name)!.asObservable();
		else
			return this.autoCompleteObservables.get(col.field.name)!;
	}
	input(col: ColumnFilter): FormControl{
		let input = this.autoCompleteInputs.get(col.field.name);
		if( input )
			return input;
		input = new FormControl();
		//let arg = this.args.get( col.field.name );
		this.autoCompleteInputs.set( col.field.name, input );
		const subject = new BehaviorSubject<string[]>([]);
		this.autoCompleteSubjects.set( col.field.name, subject );
		// this.autoCompleteSubjects.get(col.field.name).asObservable().subscribe( suggestions=> {
		// 	console.log( `slistener: ${JSON.stringify(suggestions).substring(0, 100)}` );
		// } );
		input.valueChanges.pipe(
			map( value=>{
				return this.colSuggestions(col, value);
			})
		).subscribe( suggestions=>{
			subject.next(suggestions);
		});
		subject.next( this.colSuggestions(col, "") );
		// if( this.useObservable ){
		// 	this.autoCompleteObservables.set( col.field.name, input.valueChanges.pipe(
		// 			startWith(""),
		// 			map( value=>{
		// 				console.log('oInputPipe:', value);
		// 				return this.colSuggestions(col, value);
		// 			})
		// 		)
		// 	);
		// 	this.autoCompleteValues(col).subscribe( suggestions=> {
		// 		console.log( `olistener: ${JSON.stringify(suggestions).substring(0, 100)}` );
		// 	} );
		// }
		return input;
	}
	cellClick( row:any ){
		this.selection.set( row );
	}
	isSelected( row:any ){
		return this.selection() === row;
	}
	// columnName(col: ColumnFilter): string{
	// 	return col ? StringUtils.idToDisplay(col.name) : "Selector";
	// }
	operatorSignal(col:ColumnFilter):WritableSignal<Operator>{
		return this.operatorSignals.get(col.field.name)!;
	}
	inputType( field:Field ):string{
		return field.isNumber ?  "number" : "text";
	}
	onDelete(col:ColumnFilter){
		//this.args.delete(col.field.name);
		this.autoCompleteInputs.delete(col.field.name);
		this.operatorSignals.delete(col.field.name);
		if( col.field.isNullable )
			this.nullSignals.delete(col.field.name);
		this.dataSource.splice( this.dataSource.indexOf(col), 1 );
		this.table.renderRows();
	}

	nullSignal(colName: string):WritableSignal<NullCriteria>{
		return this.nullSignals.get(colName)!;
	}
	// includeNonNull(colName): boolean{
	// 	return this.args.get( colName ).filter.value.includes("<not null>");
	// }
	// includeNull(colName): boolean{
	// 	return this.args.get( colName ).filter.value.includes("<null>");
	// }
	onNullToggle( add:boolean, col: ColumnFilter ){
		//let arg = this.args.get( col.field.name );
		if( add ){
			let notNullIndex = col.filter.value.indexOf("<not null>");
			if( notNullIndex != -1 )
				col.filter.value.splice( notNullIndex, 1 );
			col.filter.value.push("<null>");
			this.nullSignals.get( col.field.name )!.set( NullCriteria.Null );
		}else{
			let nullIndex = col.filter.value.indexOf("<null>");
			if( nullIndex != -1 )
				col.filter.value.splice( nullIndex, 1 );
			this.nullSignals.get( col.field.name )!.set( NullCriteria.None );
		}
	}
	onNonNullToggle( add:boolean, col: ColumnFilter ){
		//let arg = this.args.get( col.field.name );
		if( add ){
			let nullIndex = col.filter.value.indexOf("<null>");
			if( nullIndex != -1 )
				col.filter.value.splice( nullIndex, 1 );
			col.filter.value.push("<not null>");
			this.nullSignals.get( col.field.name )!.set( NullCriteria.NonNull );
		}else{
			let notNullIndex = col.filter.value.indexOf("<not null>");
			if( notNullIndex != -1 )
				col.filter.value.splice( notNullIndex, 1 );
			this.nullSignals.get( col.field.name )!.set( NullCriteria.None );
		}
	}
	get columnNames(){ return ["name", "operation", "filter", "delete"] };//
	get unFilteredColumns():Record<string,string>{
		let columns:Record<string,string> = {};
		for( const [name,display] of Object.entries(this.columns()) ){
			if( !this.dataSource.some(c=>c.field?.name == name) )
				columns[name] = display;
		}
		return columns;
	}
	dataSource:ColumnFilter[] = [];
	selection = signal<ViewField>( null as any );
	//args:Map<string,FieldFilter> = new Map();
	view = input.required<View>();
	columns = input.required<Record<string,string>>();
	schema = input.required<TableSchema>();
	suggestions = input.required<Record<string,any[]>>();
	excludedColumns = input.required<string[]>();
	autoCompleteSubjects = new Map<string, BehaviorSubject<string[]>>();
	autoCompleteObservables = new Map<string, Observable<string[]>>();
	operatorSignals = new Map<string, WritableSignal<Operator>>();
	nullSignals = new Map<string, WritableSignal<NullCriteria>>();

	autoCompleteInputs = new Map<string, FormControl>();
  readonly separatorKeysCodes: number[] = [ENTER, COMMA];
	@ViewChildren('dynamicInput') inputElements!: QueryList<ElementRef>;

  MyOperator = Operator;
	NullCriteria = NullCriteria;
	operatorList = Object.values(Operator);
  @ViewChild('table', {static: true}) table!: MatTable<ViewField>;
}
enum NullCriteria{
	None,
	Null,
	NonNull
}