import {EventEmitter} from '@angular/core';
import { MatTable } from '@angular/material/table';
import {Sort} from '@angular/material/sort';
import { Guid } from '../../model/Guid';
import { TraceEntry } from './TraceEntry';
import { Subject } from 'rxjs';

import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import Log = LogProto.Jde.App.Log.Proto;
export class PageStats{ constructor( public length?:number, public startIndex?:number ){} };

export class DataSource
{
	constructor( private pageSize:number=10 )
	{
	}
	connect( table:MatTable<TraceEntry> )
	{
		if( !this.observable )
		{
			this.observable = new Subject<TraceEntry[]>();
			setTimeout( ()=>{this.setPage();}, 1 );
		}
		return this.observable;
	}
	disconnect()
	{
		//console.log( "disconnect" );
		this.data.length=0;
	}
	setPage( start=-1, pageSize=0 )
	{
		if( pageSize>0 )
			this.pageSize=pageSize;
		const showEnd = start==-1 || start+pageSize>=this.data.length;
		start = showEnd ? Math.max( this.data.length-this.pageSize, 0 ) : start;
		let values = new Array<TraceEntry>();
		var end = Math.min( start+this.pageSize, this.data.length );
		for( let i=start; i<end; ++i )
			values.push( this.data[i] );
		if( this.observable )
			this.observable.next( values );
		return start;
	}
	locationOf( data:TraceEntry[], entry:TraceEntry, start:number, end:number )//https://stackoverflow.com/questions/1344500/efficient-way-to-insert-a-number-into-a-sorted-array-of-numbers
	{
		var result = end;
		if( data.length>0 && this.compare(data[end-1],entry)==1 )
		{
			if( this.compare(entry, data[start])==-1 )
				result = start;
			else
			{
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

	pushArray( entries:TraceEntry[] ):PageStats
	{
		let result = null;
		entries.forEach( (x)=>result = this.push(x) );
		return result;
	}
	push( entry:TraceEntry ):PageStats
	{
		entry.index = this.allData.length;
		let push2 = (data)=>
		{
			const location = this.locationOf( data, entry, 0, data.length );
			if( location==data.length )
				data.push( entry );
			else if( location==0 )
				data.unshift( entry );
			else
				data.splice( location, 0, entry );
		}
		push2( this.allData );
		let result = new PageStats( this.data.length );
		if( !entry.hidden )
			push2( this.data );
		if( this.autoScroll )
			result.startIndex = this.setPage();
		return result;
	}
	clear()
	{
		this.data.length=0;
		this.setPage();
	}
	compare( a:TraceEntry, b:TraceEntry )
	{
		let active = this.sort.active;
		let result = 0;
		if( !active || active=='time' )
		{
			if( a.time<b.time )
				result = -1;
			else if( a.time>b.time )
				result = 1;
			else if( a.id && b.id )
				result = a.id<b.id ? -1 : 1;
			else
				result = a.index<b.index ? -1 : 1;
		}
		else if( active=='level' )
			result = a.level==b.level ? 0 : a.level < b.level ? -1 : 1 ;
		else if( active=='message' )
			result = a.message==b.message ? 0 : a.message<b.message ? -1 : 1;
		else if( active=='file' )
			result = a.file==b.file ? 0 : a.file<b.file ? -1 : 1;
		else if( active=='function' )
			result = a.functionName==b.functionName ? 0 : a.functionName<b.functionName ? -1 : 1;
		else
			console.error( `unknown sort '${active}'` );

		return this.sort.direction === 'asc' ? result : -result;
	}
	sortData( options:Sort )
	{
		this.sort = options;
		if( !options || !options.active || options.direction === '' )
			return;

		//const values = this.data.slice();
		//const multiplier = options.direction === 'asc' ? 1 : -1;
		this.allData = this.allData.sort( (a, b) => this.compare(a,b) );
		this.data = this.data.sort( (a, b) => this.compare(a,b) );
		//let i=-1;
		//for( let row of data )
		//	row.index = ++i;

		//this.data = data;
		this.setPage();
	}
	select( dataIndex:number )
	{
		var visibleIndex = 0;
		for( ;visibleIndex<this.data.length; ++visibleIndex )
		{
			if( this.data[visibleIndex].index==dataIndex )
				break;
		}
		let page = this.page = Math.floor( visibleIndex/this.pageSize );
		let start = page*this.pageSize;
		let values = new Array<TraceEntry>();
		const end = Math.min( start+this.pageSize, this.data.length );
		for( let i=start; i<end; ++i )
			values.push( this.data[i] );
		if( this.observable )
			this.observable.next( values );
	}
	filterData( messageIds:string[], filter2:string, index:number, level:Log.ELogLevel ):PageStats{
		const filter = filter2 ? filter2.trim().toLowerCase() : null;
		//let visibleData:TraceEntry[] = [];
		this.data.length = 0;
		//let haveIndex = index==-1;
		//let pastIndex = false;
		let selectedIndex = -1;
		//let visibleIndex = 0;
		for( let entry of this.allData )
		{
			//if( !haveIndex && !pastIndex )
			//	pastIndex = entry.index==index;
			if( selectedIndex==-1 && entry.index==index )
				selectedIndex = this.data.length;
			entry.hidden = messageIds.indexOf( new Guid(entry.messageId).toString() )!=-1;
			if( entry.hidden || entry.level<level || (filter!=null && entry.message.toLowerCase().indexOf(filter)==-1) )
				continue;
			//++visibleIndex;
			//if( selectedIndex==-1 && entry.index>=index )
			//	selectedIndex = this.data.length+1;
			this.data.push( entry );
		}
		if( selectedIndex>=this.data.length )
			selectedIndex = this.data.length-1;
		//this.data = visibleData;
		this.setPage( selectedIndex );
		return new PageStats( this.data.length, selectedIndex );
	}
	applyFilter( filter:string, hiddenIds:string[] )
	{
		let visibleData:TraceEntry[] = [];
		for( let entry of this.allData )
		{
			entry.hidden = hiddenIds.indexOf(entry.messageId)!=-1;
			if( !entry.hidden && entry.message.toLowerCase().indexOf(filter)!=-1 )
				visibleData.push( entry );
		}
		this.data = visibleData;
		this.setPage();
	}
	autoScroll:boolean=true;
	get paused(){return this._paused;} set paused(value){this._paused=value;}_paused=false;
	get length():number{return this.data.length;}
	get page(){return this._page;} set page(value){this._page=value;this.onPageChange.emit(value);} _page:number; onPageChange= new EventEmitter<number>();
	//get filter(){return _filter;} set filter( value){_filter=value; applyFilter(value);} string _filter;
	sort:Sort;
	observable:Subject<TraceEntry[]>;
	data:TraceEntry[] = [];
	allData:TraceEntry[] = [];
}
