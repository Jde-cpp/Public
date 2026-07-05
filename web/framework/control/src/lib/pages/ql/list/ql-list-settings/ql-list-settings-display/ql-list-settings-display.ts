import {CdkDragDrop, CdkDropList, CdkDrag, moveItemInArray} from '@angular/cdk/drag-drop';
import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject, output, effect } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, Route, Router, Routes, UrlSegment} from '@angular/router';
import { FormsModule } from '@angular/forms';
import {Sort} from '@angular/material/sort';
import {Field} from '../../../../../model/ql/schema/Field';
import {TableSchema}  from '../../../../../model/ql/schema/TableSchema';

import { MatIcon } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatCheckbox } from '@angular/material/checkbox';
import { MatSelectModule } from '@angular/material/select';
import { MatTable, MatTableModule } from '@angular/material/table';
import { MatToolbar } from '@angular/material/toolbar';
import { Operator, View, ViewField } from '../../../../../model/ql/View';
import { StringUtils } from '../../../../../utils/StringUtils';
import { MatInput } from "@angular/material/input";
import { Schema } from '@angular/forms/signals';


@Component( {
		selector: 'ql-list-settings-display',//.main-content.mat-drawer-container.my-content
		styleUrls: ['ql-list-settings-display.scss'],
		templateUrl: './ql-list-settings-display.html',
		host: {class:'main-content.mat-drawer-container.my-content'},
		imports: [CommonModule, FormsModule, CdkDrag, CdkDropList, MatCheckbox, MatIcon, MatSelectModule, MatTableModule, MatInputModule]
})
export class QLListSettingsDisplay implements OnInit{
	ngOnInit(){
		let columns:(SelectorField|ViewField)[] = [{name:"Selector", displayed:this.view().showSelector}];
		for( let [name,display] of Object.entries(this.columns()) ){
			let field = this.view().fields.find( f=>f.name==name );
			if( field )
				columns.push( field );
			else //if( !this.excludedColumns().includes(name) )
				columns.push( new ViewField({field:{name: name, hidden:true, displayName: display}, schema:this.schema()}) );
		}
		//
		//[{name:"Selector", displayed:this.view().showSelector}, ...this.view().fields.map( f => new ViewField(f) )]
		this.dataSource = columns;
	}
	cellClick( row:any ){
		this.selection.set( row );
	}
	isSelected( row:any ){
		return this.selection() === row;
	}
	drop( event: CdkDragDrop<string> ){
		moveItemInArray( this.dataSource, event.previousIndex, event.currentIndex || 1 );
    this.table.renderRows();
	}
	columnName(col:any): string{
		return col ? StringUtils.idToDisplay(col.name) : "Selector";
	}

	get columnNames(){ return ["position", "select", "name"] };
	dataSource:(SelectorField|ViewField)[] = [];
	selection = signal<ViewField>( null as any );

	view = input.required<View>();
	columns = input.required<Record<string,string>>();
	//excludedColumns = input.required<string[]>();
	schema = input.required<TableSchema>();

	Operator = Operator;
	operatorList = Object.values(Operator);
  @ViewChild('table', {static: true}) table!: MatTable<SelectorField|ViewField>;
}
type SelectorField = {name:"Selector", displayed:boolean, sort?:number};