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
		this.#synthesizedDefault = !defaultLevel;//the row still has to show something; it just is not an override yet - see entries()
		this.dataSource = [
			{ tag: LogTags.defaultTag, level: defaultLevel ? LogTags.fromWire(defaultLevel) : ELogLevel.Information },//LogTags() in logTags.h defaults to Information when nothing is stored
			...configured,
			{...LogTags.emptyRow}
		];
	}
	isDefault( row:TagRow ):boolean{ return row.tag==LogTags.defaultTag; }
	//Picking a level is what turns the synthesized default into a real override - Information included, since the instance's
	//CONFIGURED default may be something else and choosing Information is then a deliberate change.
	onLevelChange( row:TagRow, level:ELogLevel ){
		row.level = level;
		if( this.isDefault(row) )
			this.#synthesizedDefault = false;
	}
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
	//What this sink actually overrides.  A default row the instance never stored and the user never touched is NOT one:
	//LogSettingsPanel.save diffs this against the stored rows, so reporting the synthesized Information made
	//`previous['default']` undefined != 'Information' on every Save - even one with no edits - and wrote a tag-0/Information
	//row for text, binary AND appServer, pinning the instance's default over its configured level across restarts.
	entries():Record<string,string>{
		return Object.fromEntries( this.dataSource
			.filter( r=>r.tag && !(this.#synthesizedDefault && this.isDefault(r)) )
			.map( r=>[r.tag, LogTags.toWire(r.level)] ) );
	}
	#synthesizedDefault = false;

	tags = input.required<Record<string,string>>();
	catalogue = input.required<string[]>();
	dataSource:TagRow[] = [];
	static emptyRow:TagRow = { tag: "", level: ELogLevel.Information };
	static defaultTag = "default";
	static toWire( l:ELogLevel ):string{ return l==ELogLevel.NoLog || l==ELogLevel.LogLevelNone ? "None" : ELogLevel[l]; }
	static fromWire( s:string ):ELogLevel{ return s=="None" ? ELogLevel.NoLog : (<any>ELogLevel)[s] ?? ELogLevel.Information; }
	@ViewChild('table', {static: true}) table!: MatTable<TagRow>;
}
