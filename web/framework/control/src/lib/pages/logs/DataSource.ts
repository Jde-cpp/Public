import {effect, EventEmitter, model, Signal, signal} from '@angular/core';
import { MatTable } from '@angular/material/table';
import {Sort} from '@angular/material/sort';
import { Guid } from '../../model/Guid';
//import { TraceEntry } from './TraceEntry';
import { BehaviorSubject, Observable, Subject } from 'rxjs';

import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import Log = LogProto.Jde.App.Log.Proto;
import { Entry, LogEntries, LogView } from './LogEntry';
import { verify } from '../../utils/utils';
import { DataSource } from '@angular/cdk/table';
import { CollectionViewer } from '@angular/cdk/collections';
export type PageStats = { length?:number, startIndex?:number };

type Filter = {
	messageIds:Guid[], message:string|undefined, level:Log.ELogLevel
}
export class LogDataSource extends DataSource<Entry>{
	constructor( view:Signal<LogView> ){
		super();
		this.view = view;
	}

	connect( collectionViewer: CollectionViewer ):Observable<readonly Entry[]>{
		setTimeout( ()=>{this.setPage();}, 1 );
		return this.subject.asObservable();
	}

	disconnect(collectionViewer: CollectionViewer){
		//this.entries.length=0;
	}
	getVisibleEntries( startIndex:number=0 ):Entry[]{
		let entries = [];
		let visibleIndex = 0;
		for( let entry of this.allEntries ){
			if( entry.hidden )
				continue;
			if( visibleIndex++>=startIndex )
				entries.push( entry );
			if( entries.length==this.view().limit )
				break;
		}
		return entries;
	}
	/**
	* @returns true if end of data is reached
	*/
	setPage( start=0 ):boolean{
		let pageSize = this.view().limit;
		if( start>this.allEntries.length )
			return true;
		let entries = this.getVisibleEntries( start );
		this.subject.next( entries );
		return entries.length<pageSize;
	}
	locationOf( data:Entry[], entry:Entry, start:number, end:number ):number{//https://stackoverflow.com/questions/1344500/efficient-way-to-insert-a-number-into-a-sorted-array-of-numbers
		var result = end;
		if( data.length>0 && this.compare(data[end-1],entry)==1 ){
			if( this.compare(entry, data[start])==-1 )
				result = start;
			else{
				var pivotIndex = Math.round( start + (end - start) / 2 );
				var pivot = data[pivotIndex];
				if( this.compare(entry,pivot)==this.compare(pivot,entry) )
					result = pivotIndex;
				else if( end - start <= 1 )
					result = this.compare(entry,pivot) ? pivotIndex-1 : pivotIndex;
				else if( this.compare(pivot,entry) )
					result = this.locationOf( data, entry, pivotIndex, end );
				else
					result = this.locationOf( data, entry, start, pivotIndex );
			}
		}
		return result;
	}

	addLoadedEntries( data:LogEntries ){
		//remove duplicates
		let loaded = data.entries[0];
		for( let i=this.allEntries.length-1; loaded && i>=0; --i ){//loaded is undefined for an empty batch or once shift() exhausts it — stop before dereferencing it
			let existing = this.allEntries[i];
			if( LogEntries.equals(loaded, existing) ){
				data.entries.shift();
				loaded = data.entries[0];
				i=this.allEntries.length;
			}
			else if( this.sort[0].active=='time' ){
				if( this.sort[0].direction=='desc' && loaded.time.getTime()<existing.time.getTime() )
					break;
				else if( this.sort[0].direction=='asc' && loaded.time.getTime()>existing.time.getTime() )
					break;
			}
		}
		data.entries.forEach( (x)=>this.push(x) );
		data.strings.forEach( (value,key)=>this.strings.set( key, value ) );
	}

	push( entry:Entry ):void{
		let addToArray = (data:Entry[])=>{
			const location = this.locationOf( data, entry, 0, data.length );
			if( location==data.length )
				data.push( entry );
			else if( location==0 )
				data.unshift( entry );
			else
				data.splice( location, 0, entry );
		}
		addToArray( this.allEntries );
	}
	clear(){
		this.allEntries.length=0;
		this.setPage();
	}
	compare( a:Entry, b:Entry ){
		let active = this.sort[0].active;
		let result = 0;
		if( active=='time' ){
			if( a.time<b.time )
				result = -1;
			else if( a.time>b.time )
				result = 1;
			// else if( a.id && b.id )
			// 	result = a.id<b.id ? -1 : 1;
			else
				result = 0;
		}
		else if( active=='level' )
			result = a.level==b.level ? 0 : a.level < b.level ? -1 : 1 ;
		else if( active=='message' )
			result = this.message(a)==this.message(b) ? 0 : this.message(a)<this.message(b) ? -1 : 1;
		else if( active=='file' )
			result = this.file(a)==this.file(b) ? 0 : this.file(a)!<this.file(b)! ? -1 : 1;
		else if( active=='function' )
			result = this.function(a)==this.function(b) ? 0 : this.function(a)!<this.function(b)! ? -1 : 1;
		else
			console.error( `unknown sort '${active}'` );

		return this.sort[0].direction === 'asc' ? result : -result;
	}
	file( entry:Entry ):string|undefined{ return this.strings.get( entry.fileId.toString() ); }
	function( entry:Entry ):string|undefined{ return this.strings.get( entry.functionId.toString() ); }
	message( entry:Entry ):string{
		let template = this.strings.get( entry.templateId.toString() ) ?? "";
		let text = "";
		let argIndex = 0;
		for( let i=0; i<template.length; ++i ){
			let ch = template[i];
			if( ch!='{' || i+1>=template.length || (ch=='{' && template[i+1]=='{') ){
				text += ch;
				if( ch=='{' && i+1<template.length )
					text += template[++i];
				continue;
			}
			let next = template[++i];
			if( next=='}' || i+1==template.length ){
				text += this.strings.get( entry.argIds[argIndex++].toString() );
				verify( next=='}' );
				continue;
			}
			verify( next==':' );
			let type = template[++i];
			if( type=='x' ){
				text += ( +this.strings.get( entry.argIds[argIndex++].toString())! ).toString(16);
				verify( template[++i]=='}' );
			}
			else
				verify( false, `Unknown format type '${type}' in template '${template}'` );
		}
		return text;
	}
	selectNext(){
		let templateId = null;
		let visibleIndex = 0;
		let found = false;
		for( let entry of this.allEntries ){
			if( entry.selected ){
				entry.selected = false;
				templateId = entry.templateId;
			}
			else if( templateId && entry.templateId.equals(templateId) ){
				entry.selected = true;
				found = true;
				break;
			}
			if( !entry.hidden )
				++visibleIndex;
		}
		if( found ){
			let pageSize = this.view().limit;
			let page = Math.floor( visibleIndex/pageSize );
			let start = page*pageSize;
			this.setPage( start );
		}
		return found;
	}

	isHidden( entry:Entry ):boolean{
		let filter = this.filter;
		return filter.messageIds.find( guid=>guid.equals(entry.templateId) )!=undefined
			|| entry.level<filter.level
			|| ( filter.message!=undefined && this.message(entry).toLowerCase().indexOf(filter.message)==-1 );
	}
	/*
	* @returns true if end of data is reached
	*/
	filterData( filter: Filter ):boolean{
		this.filter = filter;
		let currentVisibleIndex = 0, nextSelectedIndex = 0;
		let entries = [];
		for( let [i, entry] of this.allEntries.entries() ){
			if( entry.selected )
				nextSelectedIndex = entries.length;
			if( !entry.hidden )
				++currentVisibleIndex;
			entry.hidden = this.isHidden( entry );
			if( entry.hidden )
				entry.selected = false;
		}
		if( nextSelectedIndex==-1 )
			nextSelectedIndex = 0;
		return this.setPage( nextSelectedIndex );
	}
	autoScroll:boolean=true;
	get paused(){return this._paused;} set paused(value){this._paused=value;}_paused=false;
	filter: Filter = { messageIds: [], message: undefined, level: Log.ELogLevel.NoLog };
	// get page(){return this._page;} set page(value){this._page=value;this.onPageChange.emit(value);} _page:number;
	// onPageChange= new EventEmitter<number>();
	get pageIndex(){ return this.#pageIndex.asReadonly(); }
	#pageIndex = signal<number>( 0 );
	//get filter(){return _filter;} set filter( value){_filter=value; applyFilter(value);} string _filter;
	get sort(){ return this.view().sort; }
  private subject = new BehaviorSubject<Entry[]>([]);

	allEntries:Entry[] = [];
	strings:Map<string,string> = new Map<string,string>();
	view:Signal<LogView>;
}
