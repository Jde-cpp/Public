import { CommonModule } from '@angular/common';
import { Component, OnInit, ViewChild, input } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatIcon } from '@angular/material/icon';
import { MatSelectModule } from '@angular/material/select';
import { MatTable, MatTableModule } from '@angular/material/table';
import { MatTooltip } from '@angular/material/tooltip';
import { LogEntries } from '../log-entry';
import { SeverityPicker } from '../../../shared/severity-picker/severity-picker';

import { ELogLevel } from 'jde-proto/Log';

export type TagRow = { tag:string, level:ELogLevel };

@Component({
	selector: 'log-tags',
	templateUrl: './log-tags.html',
	styleUrls: ['./log-tags.scss'],
	imports: [CommonModule, MatButtonModule, MatFormFieldModule, MatIcon, MatSelectModule, MatTableModule, MatTooltip, SeverityPicker]
})
export class LogTags implements OnInit{
	ngOnInit(){
		const stored = this.tags();
		const configured = Object.entries( stored ).filter( ([tag])=>tag!=LogTags.defaultTag ).map( ([tag,level])=>({tag, level: LogTags.fromWire(level)}) );
		const defaultLevel = stored[LogTags.defaultTag];
		this.dataSource = [
			{ tag: LogTags.defaultTag, level: defaultLevel ? LogTags.fromWire(defaultLevel) : ELogLevel.Information },//LogTags() in logTags.h defaults to Information when nothing is stored
			...configured,
			{...LogTags.emptyRow}
		];
	}
	isDefault( row:TagRow ):boolean{ return row.tag==LogTags.defaultTag; }
	onTagChange( row:TagRow, tag:string ){
		const isNew = !row.tag;
		row.tag = tag;
		if( isNew ){
			this.dataSource.push( {...LogTags.emptyRow} );
			this.table.renderRows();
		}
	}
	onDelete( row:TagRow ){
		this.dataSource.splice( this.dataSource.indexOf(row), 1 );
		this.table.renderRows();
	}
	tagName( tag:string ):string{ return LogEntries.tagName(tag); }
	selectableTags( row:TagRow ):string[]{
		return this.catalogue().filter( t=>t==row.tag || !this.dataSource.some(r=>r.tag==t) );
	}
	entries():Record<string,string>{
		return Object.fromEntries( this.dataSource.filter(r=>r.tag).map(r=>[r.tag, LogTags.toWire(r.level)]) );
	}

	tags = input.required<Record<string,string>>();
	catalogue = input.required<string[]>();
	dataSource:TagRow[] = [];
	static emptyRow:TagRow = { tag: "", level: ELogLevel.Information };
	static defaultTag = "default";
	static toWire( l:ELogLevel ):string{ return l==ELogLevel.NoLog || l==ELogLevel.LogLevelNone ? "None" : ELogLevel[l]; }
	static fromWire( s:string ):ELogLevel{ return s=="None" ? ELogLevel.NoLog : (<any>ELogLevel)[s] ?? ELogLevel.Information; }
	@ViewChild('table', {static: true}) table!: MatTable<TagRow>;
}
