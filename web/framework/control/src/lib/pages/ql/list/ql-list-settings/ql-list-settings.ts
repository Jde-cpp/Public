import { Component, OnInit, input, signal, computed, output, ViewChild, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatTabsModule } from '@angular/material/tabs';
import { MatToolbar } from '@angular/material/toolbar';
import { QLListSettingsFilter } from './ql-list-settings-filter/ql-list-settings-filter';
import { QLListSettingsDisplay } from './ql-list-settings-display/ql-list-settings-display';
import { QLListSettingsSort } from './ql-list-settings-sort/ql-list-settings-sort';
import { View, ViewField } from '../../../../model/ql/View';
import { ProfileStore } from 'jde-spa';
import { TableSchema } from 'jde-framework';

@Component({
	selector: 'ql-list-settings',//.main-content.mat-drawer-container.my-content
	styleUrls: ['ql-list-settings.scss'],
	templateUrl: './ql-list-settings.html',
	host: {class:'main-content.mat-drawer-container.my-content'},
	imports: [CommonModule, FormsModule, MatButtonModule, MatInputModule, MatSelectModule, MatTabsModule, MatToolbar, QLListSettingsFilter, QLListSettingsDisplay, QLListSettingsSort]
})
export class QLListSettings implements OnInit, OnDestroy{
	ngOnInit(){
		this.name.set( this.originalName() );
//		this.view().append( this.schema().fields.filter(f=>!this.excludedColumns().includes(f.name)) );
	}
	ngOnDestroy(){
		ProfileStore.setTabIndex('groupDetail', this.tabIndex() );
	}

	getView( isAdhoc:boolean ):View{
		let view = this.view();  // caller should send copy of view to avoid mutating original until save
		if( !isAdhoc )
			view.name = this.name();
		let displayCols = this.display.dataSource;
		view.fields = displayCols.filter( c=>c.name!="Selector" ).map( c=>new ViewField(c as ViewField) );
		view.showSelector = displayCols[0].displayed;
		view.fieldFilters = [];
		for( let col of this.filter.dataSource.filter(c=>c.field) )
			view.fieldFilters.push( {field: col.field, filter: {operator: col.filter.operator, value: col.filter.value}} );
		view.sort = this.sort.dataSource.filter( s=>s.active ).map( s=>({active: s.active, direction: s.direction}) );
		return view;
	}

	disableSave():boolean{
		return ( this.view().isSystem && this.originalName()==this.name() )
			|| ( this.originalName()==this.name() && !this.view().name );
	}
	onTabIndexChanged( index:number ){ this.tabIndex.set(index); }
	excludedColumns = input<string[]>([]);
	suggestions = input.required<Record<string,any[]>>();
	schema = input.required<TableSchema>();
	view = input.required<View>();
	columns = input.required<Record<string,string>>();
	originalName = computed( () =>{return this.view().name ?? "<default>" + (this.view().isAdhoc ? "*" : "");} );
	onSave = output<View>();
	onShow = output<View>();
	onDelete = output<View>();
	onCancel = output<void>();

	@ViewChild(QLListSettingsDisplay) display!: QLListSettingsDisplay;
	@ViewChild(QLListSettingsFilter) filter!: QLListSettingsFilter;
	@ViewChild(QLListSettingsSort) sort!: QLListSettingsSort;

	name = signal<string>( null as any );
	tabIndex = signal<number>( ProfileStore.tabIndex('groupDetail') );
}
