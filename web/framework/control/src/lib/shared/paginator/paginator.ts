import {Component,EventEmitter,OnInit,Input,Output, OnDestroy, ChangeDetectorRef, NgModule, input, model, signal, effect, output, computed} from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
//import { BrowserModule } from '@angular/platform-browser';
import { MatInputModule } from '@angular/material/input';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { Observable, Subscription } from 'rxjs';

export class PageEvent{
	constructor( paginator:Paginator ){
		this.startIndex = paginator.startIndex();
		this.pageSize = paginator.pageSize();
	}
	startIndex:number;
	pageSize:number=50;
}

@Component({
	selector: 'paginator',
	templateUrl: './paginator.html',
	styleUrls: ['./paginator.scss'],
	imports:[MatIconModule, MatButtonModule, CommonModule, /*BrowserModule,*/ MatInputModule, FormsModule]
})
export class Paginator implements OnInit, OnDestroy{
	constructor( private cdr: ChangeDetectorRef ){
		effect( ()=>{
			this.previousPageIndex.set( this.pageIndex() );
		} );
	}
	ngOnInit(){
		//this.lengthChangeSubscription = this.lengthChange.subscribe( (value)=>this.length = value );
		//this.startIndexChangeSubscription = this.startIndexChange.subscribe( (value)=>this.startIndex = value );
	}
	ngOnDestroy(){
		//this.lengthChangeSubscription.unsubscribe();
		//this.startIndexChangeSubscription.unsubscribe();
	}
	onSelectionChange()
	{}

	onFirstPage(){
		this.startIndex.set( 0 );
		this.onPageEvent.emit( new PageEvent(this) );
	}
	onPreviousPage(){
		this.startIndex.update( value => Math.max(value-this.pageSize(), 0) );
		this.onPageEvent.emit( new PageEvent(this) );
	}
	onPrevItem(){ this.startIndex.update( value => Math.max( value-1, 0 ) ); this.onPageEvent.emit(new PageEvent(this)); }
	onNextItem(){ this.startIndex.update( value => value+1 ); this.onPageEvent.emit(new PageEvent(this)); }
	onNextPage(){ this.startIndex.update( value => value+this.pageSize() ); this.onPageEvent.emit(new PageEvent(this)); }
	onLastPage(){
		if( this.pageIndex()==Math.ceil(this.length()/this.pageSize())-1 )
			return;
		this.startIndex.update( value => this.length()-this.length()%this.pageSize() );
	}
	firstItemShowing = computed( () => this.startIndex()==0 );
	isLastPage = computed( () => this.length()!=null && this.startIndex()+this.pageSize()>=this.length() );
	onChangePageSize(value:number){
		this.pageSize.set( value );
		this.onPageEvent.emit( new PageEvent(this) );
	}
	get disabled(){ return this._disabled; }
	set disabled(value){this._disabled=value;} _disabled: boolean=false;
	hidePageLength = input<boolean>( false );
	length = input<number>( null );
	get lengthDescription(){ return this.length()!=null ? ` of ${this.length()}` : ''; }
	startIndex = model<number>(0);
/*	set length(value)
	{
		if( !value )
			value = 0;
		if( value!=this.length ){
			this._length = value;
			if( value!=0 )
				this.startIndex = this.settingsIndex || this.startIndex;
			this.settingsIndex = null;
			this.cdr.detectChanges();
		}
	} get length(){return this._length;} _length: number=0; //The length of the total number of items that are being paginated.
*/
	lengthTimeout:any;

	pageSize = model.required<number>();
	pageIndex = model<number>(0);
	previousPageIndex = signal<number>(0);
/*	//@Input() set pageIndex( value ){ this.startIndex = value*this.pageLength; } get pageIndex(){return this.startIndex()/this.length; }
	@Input() set pageLength(value){
		if( value!=this.pageLength ){
			this._pageLength=value;
			this.startIndex=this.startIndex;
			if( this.page ){
				if( this.lengthTimeout )
					clearTimeout( this.lengthTimeout );
				this.lengthTimeout = setTimeout( ()=>{
					this.lengthTimeout = undefined;
					this.page.next( {startIndex:this.startIndex(), pageLength:this.pageLength} );
				}, 1000 );
			}
		} } get pageLength(){return this._pageLength;} _pageLength:number=50;
*/
	@Input() showFirstLastButtons: boolean=true;
	//@Output() onPageEvent = new EventEmitter<PageEvent>();
	onPageEvent = output<PageEvent>();
	settingsIndex:number;
/*	@Input()	set startIndex(value){
		if( value>this.length-1 ){
			if( this.length==0 )
				this.settingsIndex = value;
			value = this.length-this.pageLength-1;
		}
		if( value<0 )
			value = 0;
		if( value!=this.startIndex ){
			this._startIndex = value;
			if( this.page )
				this.page.next( {startIndex:this.startIndex, pageLength:this.pageLength} );
		}
	}
	get startIndex(){return this._startIndex}; _startIndex:number=0;*/
	startNumber = computed( () => {
		return this.startIndex()+1;
	});
	endIndex = computed( () => {
		let endIndex = this.startIndex()+this.pageSize()-1;
		return this.length() ? Math.min( endIndex, this.length()-1 ) : endIndex;
	});
	endNumber = computed( () => {
		return this.endIndex()+1;
	});

}