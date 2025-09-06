import {Component,EventEmitter,OnInit,Input,Output, OnDestroy, ChangeDetectorRef, NgModule} from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
//import { BrowserModule } from '@angular/platform-browser';
import { MatInputModule } from '@angular/material/input';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { Observable, Subscription } from 'rxjs';

export class PageEvent{
	constructor(){this.startIndex=0; this.pageLength=50;}
	startIndex:number;
	pageLength: number;
}

@Component({
	selector: 'paginator',
	templateUrl: './paginator.html',
	imports:[MatIconModule, MatButtonModule, CommonModule, /*BrowserModule,*/ MatInputModule, FormsModule]

})
export class Paginator implements OnInit, OnDestroy{
	constructor( private cdr: ChangeDetectorRef )
	{}
	ngOnInit()
	{
		this.lengthChangeSubscription = this.lengthChange.subscribe( (value)=>this.length = value );
		this.startIndexChangeSubscription = this.startIndexChange.subscribe( (value)=>this.startIndex = value );
	}
	ngOnDestroy()
	{
		this.lengthChangeSubscription.unsubscribe();
		this.startIndexChangeSubscription.unsubscribe();
	}
	onSelectionChange(  )
	{}

	firstPage(){ this.startIndex = 0; }
	prevPage(){ this.startIndex = this.startIndex-this.pageLength; }
	prevItem(){ --this.startIndex; }
	nextItem(){ ++this.startIndex; }
	nextPage(){ this.startIndex = this.startIndex+this.pageLength; }
	lastPage(){ this.startIndex = this.length-this.length%this.pageLength; }

	get disabled(){ return this._disabled; }
	set disabled(value){this._disabled=value;} _disabled: boolean=false;
	@Input() hidePageLength: boolean;
	@Input() lengthChange:Observable<number>;	private lengthChangeSubscription: Subscription;
	@Input() startIndexChange:Observable<number>; private startIndexChangeSubscription: Subscription;
	set length(value)
	{
		if( !value )
			value = 0;
		if( value!=this.length )
		{
			this._length = value;
			if( value!=0 )
				this.startIndex = this.settingsIndex || this.startIndex;
			this.settingsIndex = null;
			this.cdr.detectChanges();
		}
	} get length(){return this._length;} _length: number=0; //The length of the total number of items that are being paginated.
	lengthTimeout;
	@Input() set pageIndex( value ){ this.startIndex = value*this.pageLength; } get pageIndex(){return this.startIndex/this.length; }
	@Input() set pageLength(value)
	{
		if( value!=this.pageLength )
		{
			this._pageLength=value;
			this.startIndex=this.startIndex;
			if( this.page )
			{
				if( this.lengthTimeout )
					clearTimeout( this.lengthTimeout );
				this.lengthTimeout = setTimeout( ()=>{ this.lengthTimeout = undefined; this.page.next( {startIndex:this.startIndex, pageLength:this.pageLength} ); }, 1000 );
			}
		} } get pageLength(){return this._pageLength;} _pageLength:number=50;
	@Input() showFirstLastButtons: boolean=true;
	@Output() page = new EventEmitter<PageEvent>();
	settingsIndex:number;
	@Input()	set startIndex(value)
	{
		if( value>this.length-1 )
		{
			if( this.length==0 )
				this.settingsIndex = value;
			value = this.length-this.pageLength-1;
		}
		if( value<0 )
			value = 0;
		if( value!=this.startIndex )
		{
			this._startIndex = value;
			if( this.page )
				this.page.next( {startIndex:this.startIndex, pageLength:this.pageLength} );
		}
	}
	get startIndex(){return this._startIndex}; _startIndex:number=0;
	get startIndexDisplay(){ return Math.max(0,Math.min(this.startIndex,this.length-1)); }
	get endIndex()
	{
		const endIndex = Math.max( Math.min(this.startIndex+this.pageLength, this.length-1), 0 );
		return endIndex;
	}
	get onFirstPage(){ return this.startIndex==0; }
	get onLastPage(){ return this.endIndex==this.length-1; }
}