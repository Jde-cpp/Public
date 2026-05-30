import {EventEmitter, model, Signal, signal} from '@angular/core';
import { MatTable } from '@angular/material/table';
import {Sort} from '@angular/material/sort';
import { Guid } from '../../model/Guid';
//import { TraceEntry } from './TraceEntry';
import { Observable, Subject } from 'rxjs';

import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import Log = LogProto.Jde.App.Log.Proto;
import { Entry, LogEntries } from './LogEntry';
import { verify } from '../../utils/utils';
import { DataSource } from '@angular/cdk/table';
import { CollectionViewer } from '@angular/cdk/collections';
export type PageStats = { length?:number, startIndex?:number };

export class LogDataSource extends DataSource<Entry>{
	constructor(){
		super();
	}

	connect( collectionViewer: CollectionViewer ):Observable<readonly Entry[]>{
		if( !this.observable ){
			this.observable = new Subject<Entry[]>();
			setTimeout( ()=>{this.setPage(0);}, 1 );
		}
		else if( this.entries.length )
			setTimeout( ()=>{this.setPage(0);}, 1 );
		return this.observable;
	}

	disconnect(collectionViewer: CollectionViewer){
		//this.entries.length=0;
	}
	setPage( start=-1, pageSize=0 ){
		if( pageSize>0 )
		 	this.pageSize = pageSize;
		const showEnd = start==-1 || start+pageSize>=this.entries.length;
		start = showEnd ? Math.max( this.entries.length-this.pageSize, 0 ) : start;
		let values = new Array<Entry>();
		var end = Math.min( start+this.pageSize, this.entries.length );
		for( let i=start; i<end; ++i )
			values.push( this.entries[i] );
		if( this.observable )
			this.observable.next( values );
		return start;
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

	pushArray( data:LogEntries ):PageStats{
		let result:PageStats = null;
		//remove duplicates
		let loaded = data.entries[0];
		for( let i=this.allEntries.length-1; i>=0; --i ){
			let existing = this.allEntries[i];
			if( LogEntries.equals(loaded, existing) ){
				data.entries.shift();
				loaded = data.entries[0];
				i=this.allEntries.length;
			}
			else if( this.sort.active=='time' ){
				if( this.sort.direction=='desc' && loaded.time.getTime()<existing.time.getTime() )
					break;
				else if( this.sort.direction=='asc' && loaded.time.getTime()>existing.time.getTime() )
					break;
			}
		}
		data.entries.forEach( (x)=>result = this.push(x) );
		data.strings.forEach( (value,key)=>this.strings.set( key, value ) );
		return { length: this.entries.length, startIndex: 0 };
	}

	push( entry:Entry ):PageStats{
		entry.index = this.allEntries.length;
		let addToArray = (data)=>{
			const location = this.locationOf( data, entry, 0, data.length );
			if( location==data.length )
				data.push( entry );
			else if( location==0 )
				data.unshift( entry );
			else
				data.splice( location, 0, entry );
		}
		addToArray( this.allEntries );
		let result:PageStats = { length: this.entries.length };
		if( !entry.hidden )
			addToArray( this.entries );
		if( this.autoScroll )
			result.startIndex = this.setPage();
		return result;
	}
	clear(){
		this.entries.length=0;
		this.setPage();
	}
	compare( a:Entry, b:Entry ){
		let active = this.sort.active;
		let result = 0;
		if( !active || active=='time' ){
			if( a.time<b.time )
				result = -1;
			else if( a.time>b.time )
				result = 1;
			// else if( a.id && b.id )
			// 	result = a.id<b.id ? -1 : 1;
			else
				result = a.index<b.index ? -1 : 1;
		}
		else if( active=='level' )
			result = a.level==b.level ? 0 : a.level < b.level ? -1 : 1 ;
		else if( active=='message' )
			result = this.message(a)==this.message(b) ? 0 : this.message(a)<this.message(b) ? -1 : 1;
		else if( active=='file' )
			result = this.file(a)==this.file(b) ? 0 : this.file(a)<this.file(b) ? -1 : 1;
		else if( active=='function' )
			result = this.function(a)==this.function(b) ? 0 : this.function(a)<this.function(b) ? -1 : 1;
		else
			console.error( `unknown sort '${active}'` );

		return this.sort.direction === 'asc' ? result : -result;
	}
	file( entry:Entry ):string{ return this.strings.get( entry.fileId.toString() ); }
	function( entry:Entry ):string{ return this.strings.get( entry.functionId.toString() ); }
	message( entry:Entry ):string{
		let template = this.strings.get( entry.templateId.toString() );
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
				text += (+this.strings.get( entry.argIds[argIndex++].toString() )).toString(16);
				verify( template[++i]=='}' );
			}
			else
				verify( false, `Unknown format type '${type}' in template '${template}'` );
		}
		return text;
	}
	sortData( options:Sort ){
		this.sort = options;
		if( !options || !options.active || options.direction === '' )
			return;

		//const values = this.entries.slice();
		//const multiplier = options.direction === 'asc' ? 1 : -1;
		this.allEntries = this.allEntries.sort( (a, b) => this.compare(a,b) );
		this.entries = this.entries.sort( (a, b) => this.compare(a,b) );
		//let i=-1;
		//for( let row of data )
		//	row.index = ++i;

		//this.entries = data;
		this.setPage();
	}
	select( dataIndex:number ){
		var visibleIndex = 0;
		for( ;visibleIndex<this.entries.length; ++visibleIndex ){
			if( this.entries[visibleIndex].index==dataIndex )
				break;
		}
		let page = Math.floor( visibleIndex/this.pageSize );
		let start = page*this.pageSize;
		this.#pageIndex.set( page );
		let values = new Array<Entry>();
		const end = Math.min( start+this.pageSize, this.entries.length );
		for( let i=start; i<end; ++i )
			values.push( this.entries[i] );
		//if( this.observable )
		//	this.observable.next( values );
	}
	filterData( messageIds:Guid[], filter2:string, index:number, level:Log.ELogLevel ):PageStats{
		const filter = filter2 ? filter2.trim().toLowerCase() : null;
		//let visibleData:TraceEntry[] = [];
		this.entries.length = 0;
		//let haveIndex = index==-1;
		//let pastIndex = false;
		let selectedIndex = -1;
		//let visibleIndex = 0;
		for( let entry of this.allEntries ){
			//if( !haveIndex && !pastIndex )
			//	pastIndex = entry.index==index;
			if( selectedIndex==-1 && entry.index==index )
				selectedIndex = this.entries.length;
			entry.hidden = messageIds.indexOf( entry.templateId )!=-1;
			if( entry.hidden || entry.level<level || (filter!=null && this.message(entry).toLowerCase().indexOf(filter)==-1) )
				continue;
			//++visibleIndex;
			//if( selectedIndex==-1 && entry.index>=index )
			//	selectedIndex = this.entries.length+1;
			this.entries.push( entry );
		}
		if( selectedIndex>=this.entries.length )
			selectedIndex = this.entries.length-1;
		//this.entries = visibleData;
		this.setPage( selectedIndex );
		return { length: this.entries.length, startIndex: selectedIndex };
	}
	applyFilter( filter:string, hiddenIds:Guid[] ){
		let visibleData:Entry[] = [];
		for( let entry of this.allEntries ){
			entry.hidden = hiddenIds.indexOf(entry.templateId)!=-1;
			if( !entry.hidden && this.message(entry).toLowerCase().indexOf(filter)!=-1 )
				visibleData.push( entry );
		}
		this.entries = visibleData;
		this.setPage();
	}
	autoScroll:boolean=true;
	get paused(){return this._paused;} set paused(value){this._paused=value;}_paused=false;
	get length():number{return this.entries.length;}
	// get page(){return this._page;} set page(value){this._page=value;this.onPageChange.emit(value);} _page:number;
	// onPageChange= new EventEmitter<number>();
	get pageIndex(){ return this.#pageIndex.asReadonly(); }
	#pageIndex = signal<number>( 0 );
	pageSize:number;
	//get filter(){return _filter;} set filter( value){_filter=value; applyFilter(value);} string _filter;
	sort:Sort;
	observable:Subject<Entry[]>;
	entries:Entry[] = [];
	allEntries:Entry[] = [];
	strings:Map<string,string> = new Map<string,string>();
}
