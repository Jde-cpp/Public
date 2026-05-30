import { Component, OnDestroy, OnInit, ViewChild, Inject, input, effect, Signal, signal, inject, computed } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTable, MatTableModule } from '@angular/material/table';
import {MatSortModule, Sort} from '@angular/material/sort';
import {MatDatepickerInputEvent} from '@angular/material/datepicker';
import { Subject, Unsubscribable } from 'rxjs';
import { LogDataSource } from './LogDataSource';
import {AppStatus} from '../../services/app/application';
import {LogSettings} from './Settings';
import { QLListSettings } from '../ql/list/ql-list-settings/ql-list-settings';
import { ComponentPageTitle } from 'jde-spa';
import {IErrorService} from '../../services/error/IErrorService';


import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import ELogLevel = LogProto.Jde.App.Log.Proto.ELogLevel;
import { FormControl } from '@angular/forms';
import { MatToolbar } from '@angular/material/toolbar';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatOption, MatSelect } from '@angular/material/select';
import { MatIcon } from '@angular/material/icon';
import { MatIconButton } from '@angular/material/button';
import {MatSelectModule} from '@angular/material/select';
import { PageEvent, Paginator } from '../../shared/paginator/paginator';
import { ProfileStore } from 'jde-spa';
import { IGraphQL, ProtoService, TableSchema, View, ViewType } from 'jde-framework';
import { Entry,LogEntries, LogEntriesRest, LogView } from './LogEntry';

// Move levels to combo.
// Add dates.
// Fix pause button.
// Comment out statuses
@Component({
	selector: 'logs',
	//.main-content.mat-drawer-container.my-content
	templateUrl: './logs.html',
	styleUrls: ['./logs.scss'],
	imports: [CommonModule, MatFormFieldModule, MatIcon, MatIconButton, MatTableModule, MatToolbar, MatSelectModule, MatSortModule, Paginator, QLListSettings]
})
export class Logs implements OnInit, OnDestroy{
	constructor( public _componentPageTitle: ComponentPageTitle, @Inject('IErrorService') private snackBar: IErrorService ){
		// effect( ()=>{
		// 	if( this.isSettings() )
		// 		debugger;
		// 	else
		// 		debugger;
		// } );
	}

	async ngOnInit(){
		this._componentPageTitle.title = "Logs";
		this.data = new LogDataSource();
		this.profile = await this.profileStore.load<LogSettings>( "logs", new LogSettings() );
		this.data.sort = this.profile.sort;
		const views = await this.profileStore.loadClassArray<LogView>( `logs/views`, LogView, LogView.schema );
		this.views = [ LogView.default(), ...views ];
		this.viewIndex = ProfileStore.viewIndex( "logs" );
		this.view.set( this.views[this.viewIndex] );
		try{
			this.load();
		}
		catch(e){
			this.snackBar.exception( e, (m)=>console.log(m) );
		}
	}
	ngOnDestroy(){
		//this.appService.statusUnsubscribe( this.statusSubscription );
		if( this.data.pageSize!=ProfileStore.pageSize("logs") )
			ProfileStore.setPageSize( "logs", this.data.pageSize );
		this.unsubscribe();
		this.profileStore.save<LogSettings>( "logs", this.profile );
	}

	async load( startIndex:number=0 ){
		let orderBy = null;
		if( this.data.sort && this.data.sort.active ){
			orderBy = {};
			orderBy[this.data.sort.active] = this.data.sort.direction;
		}
		//TODO use skip variable.
		const limit = this.view().limit ?? LogView.default().limit;
		const vars = {limit: limit*3, skip: this.data.length, orderBy: orderBy };
		const q = "logs( limit: $limit, skip: $skip, orderBy: $orderBy ){ entries{templateId argIds level tags line time userId fileId functionId} strings{id value} }";
		const entries = ( await this.service().ql<{logs: LogEntriesRest}>( this.view().query(), (m)=>console.log(m) ) ).logs;
		this.push( new LogEntries(entries) );
		this.data.setPage( startIndex, limit );
		this.isLoading.set( false );
	}
/*	onTrace = async ( trace:FromServer.ITrace ):Promise<void> =>{
		//let status = this.applications.find( (app)=>{return app.id==trace.InstanceId;} );
		//if( !status )
		//	throw `no status for ${trace.InstanceId}`;
		let entry = new TraceEntry( trace, this.applicationStrings );
		var stringRequests = this.applicationStrings.requests( entry );
		const haveRequest = stringRequests.files.length || stringRequests.functions.length || stringRequests.messages.length || stringRequests.userPKs.length;
		// if( haveRequest )
		// 	this.onStrings( await this.appService.requestStrings(stringRequests) );

		entry.hidden = this.profile.hiddenMessages.indexOf(entry.messageId)!=-1;
		if( haveRequest || this.buffer.length )
			this.buffer.push( entry );
		else
			this.push( [entry] );
	}*/
	push( entries:LogEntries ){
		this.data.pushArray( entries );
	}
	onStrings = ( value:FromServer.Strings ):void =>{
//		if( value )
//			this.applicationStrings.set( value );
		let i=0;
		let entries = [];
		for( ; i<this.buffer.length; ++i ){
			let entry = this.buffer[i];
			const haveStrings = entry.templateId && !entry.fileId && !entry.functionId;
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
			//this.push( entries );
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
			this.buffer.length=0;
			this.data.clear();
			this.startIndexChange.next( 0 );
			this.lengthChange.next( 0 );
			this.unsubscribe();
			this.level = level;
			this.currentSubscription = subscription;
			//this.subscription = this.appService.logs( subscription.applicationId, subscription.level, subscription.start, subscription.limit ).subscribe( traces => {this.onTrace(traces);} );
		}
	}

	unsubscribe(){
		if( this.subscription ){
			this.subscription.unsubscribe();
			this.subscription = null;
			this.currentSubscription = Logs.DefaultSubscription;
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
  	}
	onPagerChange( event:PageEvent ){
		//const offset = event.pageIndex * event.pageSize;
		if( this.selectedEntry ){
			let index = this.data.entries.findIndex( (x)=>x.index==this.selectedIndex );
			if( index<event.startIndex || index>event.startIndex+event.pageSize )
				this.selectedEntry = null;
		}
		if( event.startIndex+event.pageSize>this.data.length )
			this.load( event.startIndex );
		else
			this.data.setPage( event.startIndex, event.pageSize );
	}
	onViewChange(index:number){
		this.viewIndex = index;
		ProfileStore.setViewIndex( "logs", index );
		this.view.set( this.views[index] );
		this.load();
	}
	onViewSave($event:View){
		debugger;
	}
	onViewShow(view:View){
		this.data.clear();
		this.isSettings.set( false );
		if( view.name?.endsWith("*") && view.isAdhoc )
			view.name = view.name.substring( 0, view.name.length-1 );
		view.type = ViewType.Adhoc;
		let existing = this.views.findIndex( v=>v.name==view.name && view.type==v.type );
		if( existing>=0 ){
			this.views[existing] = new LogView( view );
			this.viewIndex = existing;
		}
		else{
			this.views.push( new LogView(view) );
			this.viewIndex = this.views.length - 1;
		}
		this.load();
	}
	onViewDelete($event:View){
		debugger;
	}

	cellClick( event ){
		const row = event.target.parentElement as Element;
		var index = +row.attributes["indx"].nodeValue;
		this.selectedIndex = index==this.selectedIndex ? null : index;
	}
	hideSelectedMessage(){
		this.profile.level = ELogLevel.Information;
		this.profile.hiddenMessages.push( this.selectedEntry.templateId );
		this.profile.level = ELogLevel.Debug;
		this.filterData();
	}
	clearHiddenMessages(){
		this.profile.hiddenMessages.length=0;
		this.filterData();
	}
	filterData(){
		var changes = this.data.filterData( this.profile.hiddenMessages, this.filter, this.selectedEntry ? this.selectedEntry.index : -1, this.level );
		if( changes.length )
			this.lengthChange.next( changes.length );
		if( changes.startIndex ){
			this.selectedEntry = this.data.entries[changes.startIndex];
			this.startIndexChange.next( changes.startIndex );
		}
	}
	navigateNext(){
		const messageId = this.selectedEntry.templateId;
		const currentIndex = this.data.entries.findIndex( (x)=>x.index==this.selectedIndex );
		const size = this.data.entries.length;
		const stop = size+currentIndex;
		let foundIndex = currentIndex;
		for( let i=currentIndex+1; i!=stop && foundIndex==currentIndex; ++i ){
			const i2 = i%size;//<size ? i : i-size;
			if( this.data.entries[i2].templateId==messageId )
				foundIndex = i2;
		}
		if( foundIndex!=currentIndex ){
			this.selectedEntry = this.data.entries[foundIndex];
			this.data.select( foundIndex );
		}
		else
			this.snackBar.warn( "No other instances found.", (m)=>console.log(m) );
	}
	applyFilter( value:string ){
		this.filter = value;
		this.filterData();
	}
	get sort(){return this.profile.sort;} set sort(value){ this.data.sort = this.profile.sort = value; }

	service = input.required<IGraphQL>();

	profile:LogSettings;
//	get settings(){ return this.profile;}


	//settings:Settings = new Settings();
	data: LogDataSource;
	get paused(){return this.data.paused;} set paused(value){this.data.paused=value;}
	connected = false;
	displayedColumns = computed( () => {
		return this.view().fields.map( (f)=>f.name );
	} );
	//configuration = { displayHeader:true }
	@ViewChild('mainTable',{static: false}) _table:MatTable<Entry>;

	toLevel( level:ELogLevel ):string{
		switch( level ){
			case ELogLevel.Trace: return "Trc";
			case ELogLevel.Debug: return "Dbg";
			case ELogLevel.Information: return "Inf";
			case ELogLevel.Warning: return "Wrn";
			case ELogLevel.Error: return "Err";
			case ELogLevel.Critical: return "Crt";
		}
		return "";
	}
	levelClass(row:Entry){
		let className = "";
		//const levelValue = ELogLevel[row.level as keyof typeof ELogLevel];
		switch( row.level ){
			case ELogLevel.Trace: className = "log-trace"; break;
			case ELogLevel.Debug: className = "log-debug"; break;
			case ELogLevel.Information: className = "log-information"; break;
			case ELogLevel.Warning: className = "log-warning"; break;
			case ELogLevel.Error: className = "log-error"; break;
			case ELogLevel.Critical: className = "log-critical"; break;
		}
		return "table-row "+className;
	}
	message(entry):string{
		return this.data.message(entry);
	}

	get applicationId(){ return this.profile.applicationId; } set applicationId(value){ this.profile.applicationId=value; }
	get columns():Record<string,string>{ return LogEntries.columns; }
	get start():Date{ return this._start.value; } set start(value:Date){ this._start.setValue(value); this.profile.start = value; } private _start = new FormControl();
	private filter:string; 	//get filter(){return _filter;} set filter(value){ this._filter = value.trim().toLowerCase(); }
	startChange( event: MatDatepickerInputEvent<Date> ){ this.subscribe( this.applicationId, this.level ); }
	private buffer:Entry[] = [];
	static DefaultSubscription:ISubscription={ applicationId: 0, level:  ELogLevel.NoLog, start:null };
	private currentSubscription:ISubscription=Logs.DefaultSubscription;//actual subscribtion
	isLoading = signal<boolean>( true );
	isSettings = signal<boolean>( false );
	lengthChange = new Subject<number>();
	startIndexChange = new Subject<number>();
	get level():ELogLevel{ return this.profile.level; } set level( value:ELogLevel ){ this.profile.level=value; }
	private get limit():number{return this.profile.limit;} private set limit(value:number){ this.profile.limit = value; }
	private get application():AppStatus|null{ return this.applications.find( (existing)=>{return existing.id==this.applicationId;} ); }
	applications:AppStatus[]=[];
	schema:TableSchema = LogView.schema;
	private subscription:Unsubscribable;
	//private applicationStrings:ApplicationStrings = new ApplicationStrings();
	//private pushTimeout:{ entries: TraceEntry[], id:any, end:number };
	get selectedIndex(){ return this.selectedEntry?.index; } set selectedIndex(x){ this.selectedEntry = this.data.entries.find( (y)=>y.index==x ); }
	get selectedEntry(){return this._selectedEntry; } set selectedEntry(x){ this._selectedEntry=x;} _selectedEntry:Entry;
	views:LogView[];
	view = signal<LogView>( null );
	viewIndex:number;
	profileStore = inject(ProfileStore);
}

interface ISubscription{ applicationId:number, level:ELogLevel, start:Date|null }