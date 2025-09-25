import {Component, Input, Output, OnInit, EventEmitter, NgModule,ViewChild} from '@angular/core';
import { FormControl, FormGroup } from '@angular/forms';
import {MatChipListbox, MatChipOption, MatChipsModule} from '@angular/material/chips';
import {MatDatepickerInputEvent, MatDatepickerModule, MatDatepickerToggle, MatDateRangeInput, MatDateRangePicker} from '@angular/material/datepicker';
import {MatFormField, MatFormFieldModule, MatLabel} from '@angular/material/form-field';

import { DateUtilities, Day } from '../../utilities/dateUtilities';
import { IAssignable } from '../../utilities/settings';

export enum TimeFrame{None=0, Week=7, Month=30, Quarter=90, Year=360, All=1000}

@Component( {
	selector: 'date-range',
	templateUrl: 'date-range.html',
	imports: [MatFormField, MatLabel, MatDateRangeInput, MatDatepickerToggle, MatDateRangePicker,MatChipListbox,MatChipOption]} )
export class DateRange implements OnInit
{
	get startDate()
	{
		let d = DateRange.toDate( this.settings.start );
		//console.log( `start=${d}` );
		return d;
	}
	get endDate(){ return this.settings.end ? DateRange.toDate( this.settings.end ) : null; }
	@ViewChild('picker',{static: false}) picker;
	@ViewChild('dateRangeStart',{static: false}) dateRangeStart;
	@ViewChild('dateRangeEnd',{static: false}) dateRangeEnd;
	@ViewChild('dateRange',{static: false}) dateRange;
	ngOnInit()
	{
		if( this.settings.timeFrame )
			this.setTimeFrame( this.settings.timeFrame, true, false );
		else
		{
			//this.end.value = this.settings.end;
			let x  = this.picker;
			let a  = this.dateRangeStart;
			let b  = this.dateRangeEnd;

			this.start = new FormControl( this.settings.start );
			this.end = new FormControl( this.settings.end );
		}

	/*	this.range.controls['start'].valueChanges.subscribe( value=>
		{
			debugger;
			const day = value==null ? null : DateUtilities.toDays( value );
			if( day!=this.start )
			{
				console.log( `_start=${day}` );
				this.timeFrame = TimeFrame.None;
				this.settings.start = day;
				this.settingsChange.emit( this.settings );
			}
		});
		this.range.controls['end'].valueChanges.subscribe( value=>
		{
			debugger;
			const day = value==null ? null : DateUtilities.toDays( value );
			if( day!=this.end )
			{
				this.timeFrame = TimeFrame.None;
				this.settings.end = day;
				this.settingsChange.emit( this.settings );
			}
		});
*/
	}
	static toDate( day:Day ):Date
	{
		const time=DateUtilities.fromDays(day);
		return new Date( time.getTime()+time.getTimezoneOffset()*60000 );
	}
	static fromDate( time:Date ):Day
	{
		return DateUtilities.toDays( new Date(time.getTime()-time.getTimezoneOffset()*60000) );
	}
	startChange( e:MatDatepickerInputEvent<Date>|any )
	{
		console.log( `startChange = ${e.value}  - start=${this.settings.start} - end=${this.settings.end ?? this.settings.max}, days=${this.settings.dayCount}` );
		this.settings.timeFrame = null;
		const days = DateUtilities.toDays( e.value );//18,962=12/1 18,983=12/22, 18,990=12/29
		this.settings.start = days;
		console.log( `start=${this.settings.start} - end=${ this.settings.end ?? this.settings.max }, days=${this.settings.dayCount}` );
		let d = DateRange.toDate( this.settings.start );
		console.log( `event=${e.value} - start=${d}, days=${this.settings.start}` );
//		console.log( 'start='+this.range.controls['start'].value );
//		console.log( 'end='+this.range.controls['end'].value );
	}
	endChange( e:MatDatepickerInputEvent<Date>|any ){
		console.log( `endChange = ${e.value}  - start=${this.settings.start} - end=${this.settings.end ?? this.settings.max}, days=${this.settings.dayCount}` );
		this.settings.end = e.value ? DateUtilities.toDays( e.value ) : null;
		//console.log( 'start='+this.range.controls['start'].value );
		//console.log( 'end='+this.range.controls['end'].value );
		if( this.settings.end )
			this.settingsChange.emit( this.settings );
	}
	setStartControl(day:Day){ this.range.controls['start'].setValue( day==null ? null : DateRange.toDate(day) ); }
	setEndControl(day:Day){ this.range.controls['end'].setValue( day ? DateRange.toDate(day) : null ); }
	setTimeFrame( x:TimeFrame, force=false, sendChange=true )
	{
		if( x==TimeFrame.None || (x==this.timeFrame && !force) )
			return;
		this.timeFrame = x;
		this.settings.dayCount = this.settings.end = undefined;
		if( !this.end )
			this.end = new FormControl( new Date() );
		else
			this.end.setValue( new Date() );
		if( !this.start )
			this.start = new FormControl( this.end.value.getDate()-this.timeFrame );
		else
			this.start.setValue( this.end.value.getDate()-this.timeFrame );
		if( sendChange )
			this.settingsChange.emit( this.settings );
	}
	get dayCount(){ return this.settings.dayCount; }
	@Input() set placeholder( value ){ this._placeholder = value;} get placeholder(){return this._placeholder} private _placeholder:string="Date range";
	@Input()settings:DateRangeSettings; @Output("change") settingsChange = new EventEmitter<DateRangeSettings>();
	start:FormControl;
	end:FormControl;
	get timeFrame(){return this.settings.timeFrame;} set timeFrame(x){this.settings.timeFrame = x;}TimeFrameType = TimeFrame;
	range = new FormGroup( { start: new FormControl(), end: new FormControl() } );
}
export class DateRangeSettings implements IAssignable<DateRangeSettings>
{
	constructor( public timeFrame:TimeFrame=undefined, private _max:Day = DateUtilities.toDays(new Date()), private isValidDay:(_:Day)=>boolean=()=>{return true;} )
	{}
	assign(other: DateRangeSettings)
	{
		this.end = other.end;
		this.start = other.start;
		this.timeFrame = other.timeFrame;
	}
	toJSON = function()
	{
		return { start: this.start, end: this.end, timeFrame: this.timeFrame };
  	}
	get max(){ return this._max; }
	get dayCount()
	{
		let dayCount = this.#dayCount ?? null;
		if( this.timeFrame==TimeFrame.Week )
			dayCount = 7;
		else if( this.timeFrame && this.timeFrame!=TimeFrame.All )
		{
			const max:Date = DateUtilities.fromDays( this.max );
			const delta = this.timeFrame==TimeFrame.Month ? 1 : this.timeFrame==TimeFrame.Quarter ? 3 : 12;
			const start = new Date( max.getUTCFullYear(), max.getUTCMonth()-delta, max.getUTCDate() );
			dayCount = this.max - DateUtilities.toDays( start );
		}
		return dayCount;
	} set dayCount( x:number|undefined ){ this.#dayCount = x; } #dayCount:number | undefined;
	get end(){ return this.#end; } set end( x:Day ){ this.#end = x; }  #end:Day;

	get start(){ return this.#start; } set start( x:Day ){ this.#start = x; }  #start:Day;
}
/*
@NgModule( {exports: [DateRange], declarations: [DateRange], imports:[MatChipsModule,MatDatepickerModule,MatFormFieldModule]} )
export class DateRangeModule
{}
*/