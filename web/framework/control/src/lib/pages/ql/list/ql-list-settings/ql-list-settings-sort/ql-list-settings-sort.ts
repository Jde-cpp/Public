import { CommonModule } from "@angular/common";
import { Component, input, OnInit, ViewChild } from "@angular/core";
import { FormsModule } from "@angular/forms";
import { MatAutocompleteModule } from "@angular/material/autocomplete";
import { MatButtonModule } from "@angular/material/button";
import { MatIcon } from "@angular/material/icon";
import { MatInputModule } from "@angular/material/input";
import { MatSelectModule } from "@angular/material/select";
import { Sort } from "@angular/material/sort";
import { MatTable, MatTableModule } from "@angular/material/table";
import { TableSchema, View } from "jde-framework";


@Component( {
		selector: 'ql-list-settings-sort',//.main-content.mat-drawer-container.my-content
		templateUrl: './ql-list-settings-sort.html',
		styleUrls: ['./ql-list-settings-sort.scss'],
		host: {class:'main-content.mat-drawer-container.my-content'},
		imports: [CommonModule, FormsModule, MatAutocompleteModule, MatButtonModule, MatIcon, MatInputModule, MatSelectModule, MatTableModule]
})
export class QLListSettingsSort implements OnInit{
	ngOnInit(){
		this.dataSource = [...this.view().sort, {...QLListSettingsSort.emptySort}];
	}
	onToggleDirection( col:Sort ){
		col.direction = col.direction === 'asc' ? 'desc' : 'asc';
	}
	onDelete(element:Sort){
		const index = this.dataSource.indexOf(element);
		this.dataSource.splice(index, 1);
    this.table.renderRows();
	}

	dataSource:Sort[] = [];
	view = input.required<View>();
	schema = input.required<TableSchema>();
	excludedColumns = input.required<string[]>();
	updateSort(element:Sort, value:string){
		const isNew = !element.active;
		element.active = value;
		element.direction = 'asc';
		if( isNew ){
			this.dataSource.push( {...QLListSettingsSort.emptySort} );
			this.table.renderRows();
		}
	}
	selectableFields( sort:Sort ):any{
		return Object.fromEntries(
			this.schema().fields.filter( f=>
				f.name == sort.active ||
				(!this.excludedColumns().includes(f.name)
				&& !this.dataSource.some(s=>s.active==f.name) )
			).map( f=>[f.name, f.name] )
		);
	}
	static emptySort:Sort = {active: "", direction: 'asc'};
	@ViewChild('table', {static: true}) table!: MatTable<Sort>;
}