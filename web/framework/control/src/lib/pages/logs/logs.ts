import { HostListener, Component, OnDestroy, OnInit, ViewChild, Inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTable, MatTableModule } from '@angular/material/table';
import {MatSortModule, Sort} from '@angular/material/sort';
import {MatDatepickerInputEvent} from '@angular/material/datepicker';
import { Observable, Subject, Unsubscribable } from 'rxjs';
import { TraceEntry } from './TraceEntry';
import { DataSource } from './DataSource';
import {App,AppStatus} from '../../services/app/application';
import {AppService} from '../../services/app/app.service';
import {ApplicationStrings} from './Application';
import {LogSettings} from './Settings';
import {Settings} from '../../utilities/settings';
import { ComponentPageTitle } from 'jde-spa';
import {IProfile} from '../../services/profile/IProfile';
import {IErrorService} from '../../services/error/IErrorService';


import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as AppFromClient from '../../proto/App.FromClient'; import FromClient = AppFromClient.Jde.App.Proto.FromClient;
import * as CommonProto from '../../proto/Common'; import ELogLevel = CommonProto.Jde.Proto.ELogLevel;
import { FormControl } from '@angular/forms';
import { MatToolbar } from '@angular/material/toolbar';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatOption, MatSelect } from '@angular/material/select';
import { MatIcon } from '@angular/material/icon';
import { Paginator } from '../../shared/paginator/paginator';

// Move levels to combo.
// Add dates.
// Fix pause button.
// Comment out statuses
@Component({
	selector: 'logs.main-content.mat-drawer-container.my-content',
	templateUrl: './logs.html',
	styleUrls: ['./logs.css'],
	imports: [CommonModule, MatIcon, MatTableModule, MatToolbar, MatFormFieldModule, MatSelect, MatSortModule, MatOption, Paginator]
})
export class LogsComponent implements OnInit, OnDestroy{
	constructor( public _componentPageTitle: ComponentPageTitle, private appService:AppService, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private errorService: IErrorService ){
		this.settingsContainer = new Settings<LogSettings>( LogSettings, "LogComponent", this.profileService );
	}

	async ngOnInit(){
		this._componentPageTitle.title = "Logs";
		/*var beginningOfDay = DateUtilities.beginningOfDay( new Date() );
		beginningOfDay.setDate( beginningOfDay.getDate()-1 );
		//var yesterday = ;
		var start = beginningOfDay;
		this._start.setValue( start );*/
		this.data.onPageChange.subscribe( pageIndex=>this.pageIndex = pageIndex );
		await this.settingsContainer.loadedPromise;

		this.data.sort = this.settings.sort;
		try{
			let applications = await this.appService.query<App[]>( "applications{id name dbLogLevel fileLogLevel}" );
			for( let app of applications )
				this.applications.push( new AppStatus(app) );

			this.statusSubscription = this.appService.statuses();
			this.statusSubscription.subscribe( (status:FromServer.IStatus) =>{
				let found = this.applications.find( (existing)=>{return existing.id==status.applicationId;} );
				if( !found )
					console.error( `Could not find application '${status.applicationId}'` );
				else
					found.status = status;
			} );
			this.subscribe( this.applicationId, this.level );
			this.viewPromise = Promise.resolve( true );
		}
		catch(e){
			console.log(e);
		}
	}
	ngOnDestroy(){
		this.appService.statusUnsubscribe( this.statusSubscription );
		this.unsubscribe();
		this.settingsContainer.save();
	}


	onTrace = async ( trace:FromServer.ITrace ):Promise<void> =>{
		//let status = this.applications.find( (app)=>{return app.id==trace.InstanceId;} );
		//if( !status )
		//	throw `no status for ${trace.InstanceId}`;
		let entry = new TraceEntry( trace, this.applicationStrings );
		var stringRequests = this.applicationStrings.requests( entry );
		const haveRequest = stringRequests.files.length || stringRequests.functions.length || stringRequests.messages.length || stringRequests.userPKs.length;
		if( haveRequest )
			this.onStrings( await this.appService.requestStrings(stringRequests) );

		entry.hidden = this.settings.hiddenMessages.indexOf(entry.messageId)!=-1;
		if( haveRequest || this.buffer.length )
			this.buffer.push( entry );
		else
			this.push( [entry] );
	}
	push( entries:TraceEntry[] ){
		let fnctn = ( entries2:TraceEntry[] )=>{
			var changes = this.data.pushArray( entries2 );
			if( changes.length )
				this.lengthChange.next( changes.length );
			if( changes.startIndex )
				this.startIndexChange.next( changes.startIndex );
			this.pushTimeout = null;
		};
		const now = new Date();
		let timeout = this.pushTimeout==null;
		if( !timeout ){
			entries = this.pushTimeout.entries.concat( entries );
			clearTimeout( this.pushTimeout.id );
			timeout = this.pushTimeout.end>now.getTime();
			if( !timeout )
				fnctn( entries );
		}
		if( timeout ){
			this.pushTimeout ={
				entries: entries,
				end: this.pushTimeout ? this.pushTimeout.end : now.getTime()+1000,
				id: setTimeout( ()=>{ fnctn(entries) }, 250 )
			};
		}
	}
	onStrings = ( value:FromServer.Strings ):void =>{
		if( value )
			this.applicationStrings.set( value );
		let i=0;
		let entries = [];
		for( ; i<this.buffer.length; ++i ){
			let entry = this.buffer[i];
			const haveStrings = (entry.messageId || entry.message!=null) && (!entry.fileId || entry.file!=null) && ( !entry.functionId || entry.functionName!=null );
			//haveStrings = entry.message!=null && entry.file!=null && entry.functionName!=null;
			if( haveStrings )
				entries.push( entry );
			else{
				//console.log( `~(${entry.messageId}) haveStrings='${haveStrings}' message='${entry.message}' && ${entry.file} && ${entry.functionName} - ${entry.lineNumber}, buffer.length=${this.buffer.length-i}` );
				break;
			}
		}
		if( i>0 ){
			this.buffer.splice( 0, i );
			this.push( entries );
		}
	//	if( !this.buffer.length )
	//		console.log( 'no buffer length' );
	}

	onChangeApplication( event:number ){
		//if( event.source.selected )
			this.subscribe( event, this.level );
	}
	subscribe( applicationId:number, level:ELogLevel ){
		var subscription = { applicationId: applicationId, level: level, start:this.start, limit:this.limit };
		if( JSON.stringify(this.currentSubscription)!=JSON.stringify(subscription) ){
			//this.profileService.put<Settings>( LogsComponent.profileKey, this.settings );
			this.buffer.length=0;
			this.data.clear();
			this.startIndexChange.next( 0 );
			this.lengthChange.next( 0 );
			this.unsubscribe();
			this.level = level;
			this.currentSubscription = subscription;
			this.settingsContainer.save();
			this.subscription = this.appService.logs( subscription.applicationId, subscription.level, subscription.start, subscription.limit ).subscribe( traces => {this.onTrace(traces);} );
		}
	}

	unsubscribe(){
		if( this.subscription ){
			this.subscription.unsubscribe();
			this.subscription = null;
			this.currentSubscription = LogsComponent.DefaultSubscription;
		}
	}
	// @HostListener('window:scroll', ['$event'])
	// doSomething(event)
	// {
	// 	console.debug("Scroll Event", document.body.scrollTop );
	// 	console.debug("Scroll Event", window.pageYOffset );
	// }
	onLevelChange( logLevel:ELogLevel ){
		this.subscribe( this.applicationId, logLevel );
	}
	//@ViewChild("table-body") configuration:ConfigureTableComponent;
	sortData(sort: Sort|any){
		this.data.sortData( sort );
		this.sort = sort;
		this.settingsContainer.save();
  	}
	pageChangeEvent( event ){
		//const offset = event.pageIndex * event.pageSize;
		if( this.selectedEntry ){
			let index = this.data.data.findIndex( (x)=>x.index==this.selectedIndex );
			if( index<event.startIndex || index>event.startIndex+event.pageLength )
				this.selectedEntry = null;
		}
		this.data.setPage( event.startIndex, event.pageLength );
	}
	cellClick( event ){
		const row = event.target.parentElement as Element;
		var index = +row.attributes["indx"].nodeValue;
		this.selectedIndex = index==this.selectedIndex ? null : index;
	}
	hideSelectedMessage(){
		this.settings.level = ELogLevel.Information;
		this.settings.hiddenMessages.push( this.selectedEntry.messageId );
		this.settings.level = ELogLevel.Debug;
		this.settingsContainer.save();
		this.filterData();
	}
	clearHiddenMessages(){
		this.settings.hiddenMessages.length=0;
		this.filterData();
	}
	filterData(){
		var changes = this.data.filterData( this.settings.hiddenMessages, this.filter, this.selectedEntry ? this.selectedEntry.index : -1, this.level );
		if( changes.length )
			this.lengthChange.next( changes.length );
		if( changes.startIndex ){
			this.selectedEntry = this.data.data[changes.startIndex];
			this.startIndexChange.next( changes.startIndex );
		}
	}
	navigateNext(){
		const messageId = this.selectedEntry.messageId;
		const currentIndex = this.data.data.findIndex( (x)=>x.index==this.selectedIndex );
		const size = this.data.data.length;
		const stop = size+currentIndex;
		let foundIndex = currentIndex;
		for( let i=currentIndex+1; i!=stop && foundIndex==currentIndex; ++i ){
			const i2 = i%size;//<size ? i : i-size;
			if( this.data.data[i2].messageId==messageId )
				foundIndex = i2;
		}
		if( foundIndex!=currentIndex ){
			this.selectedEntry = this.data.data[foundIndex];
			this.data.select( foundIndex );
		}
		else
			this.errorService.warn( "No other instances found.", (m)=>console.log(m) );
	}
	applyFilter( value:string ){
		this.filter = value;
		this.filterData();
	}
	get sort(){return this.settings.sort;} set sort(value){ this.data.sort = this.settings.sort = value; }
	settingsContainer:Settings<LogSettings>;
	get settings(){ return this.settingsContainer.value;}


	//settings:Settings = new Settings();
	pageSize:number=23;
	pageIndex:number=0;
	data: DataSource = new DataSource( this.pageSize );
	get paused(){return this.data.paused;} set paused(value){this.data.paused=value;}
	connected = false;
	displayedColumns : string[] = [ 'time', 'level', 'message', 'function', 'file', 'line' ];
	//configuration = { displayHeader:true }
	@ViewChild('mainTable',{static: false}) _table:MatTable<TraceEntry>;

	toLevel( level:ELogLevel ):string{ return ELogLevel[level]; }

	get applicationId(){ return this.settings.applicationId; } set applicationId(value){ this.settings.applicationId=value; }
	get start():Date{ return this._start.value; } set start(value:Date){ this._start.setValue(value); this.settings.start = value; } private _start = new FormControl();
	private filter:string; 	//get filter(){return _filter;} set filter(value){ this._filter = value.trim().toLowerCase(); }
	startChange( event: MatDatepickerInputEvent<Date> ){ this.subscribe( this.applicationId, this.level ); }
	private buffer:TraceEntry[] = [];
	static DefaultSubscription:ISubscription={ applicationId: 0, level:  ELogLevel.NoLog, start:null };
	private currentSubscription:ISubscription=LogsComponent.DefaultSubscription;//actual subscribtion
	lengthChange = new Subject<number>();
	startIndexChange = new Subject<number>();
	get level():ELogLevel{ return this.settings.level; } set level( value:ELogLevel ){ this.settings.level=value; }
	private get limit():number{return this.settings.limit;} private set limit(value:number){ this.settings.limit = value; }
	private get application():AppStatus|null{ return this.applications.find( (existing)=>{return existing.id==this.applicationId;} ); }
	applications:AppStatus[]=[];
	private subscription:Unsubscribable;
	private applicationStrings:ApplicationStrings = new ApplicationStrings();
	private pushTimeout:{ entries: TraceEntry[], id:any, end:number };
	get selectedIndex(){ return this.selectedEntry?.index; } set selectedIndex(x){ this.selectedEntry = this.data.data.find( (y)=>y.index==x ); }
	get selectedEntry(){return this._selectedEntry; } set selectedEntry(x){ this._selectedEntry=x;} _selectedEntry:TraceEntry;
	private statusSubscription:Observable<FromServer.IStatus>;//TODO make sure unsubscibing
	viewPromise:Promise<boolean>;
}

interface ISubscription{ applicationId:number, level:ELogLevel, start:Date|null }