import { Component, OnDestroy, OnInit, ViewChild, Inject, input, effect, Signal, signal, inject, computed } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTable, MatTableModule } from '@angular/material/table';
import {MatSortModule, Sort} from '@angular/material/sort';
import {MatDatepickerInputEvent} from '@angular/material/datepicker';
import { Subject, Unsubscribable } from 'rxjs';
import { LogDataSource } from './DataSource';
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
import { IGraphQL, ProtoService, TableSchema, verify, View, ViewType } from 'jde-framework';
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
		this.data = new LogDataSource( this.view );
		this.profile = await this.profileStore.load<LogSettings>( "logs", new LogSettings() );
		const views = await this.profileStore.loadClassArray<LogView>( `logs/views`, LogView, LogView.schema );
		this.views.set( [ LogView.default(), ...views ] );
		this.viewIndex.set( Math.min(ProfileStore.viewIndex("logs"), this.views().length - 1) );
		this.load();
	}
	ngOnDestroy(){
		//this.appService.statusUnsubscribe( this.statusSubscription );
		this.unsubscribe();
		this.profileStore.save<LogSettings>( "logs", this.profile );
	}

	async load( startIndex:number=0 ){
		try{
			const entries = ( await this.service().ql<{logs: LogEntriesRest}>( this.view().query(undefined,startIndex), (m)=>console.log(m) ) ).logs;
			if( entries?.entries?.length )//was Object.keys(entries).length, always >=2 ({entries,strings}); check the actual entry count
				this.push( new LogEntries(entries) );
			this.data.setPage( startIndex );
			this.isLoading.set( false );
		}
		catch(e){
			this.snackBar.exception( e, (m)=>console.log(m) );
		}
	}

	push( entries:LogEntries ){
		this.data.addLoadedEntries( entries );
	}
	onStrings = ( value:FromServer.Strings ):void =>{
		let i=0;
		let entries = [];
		for( ; i<this.buffer.length; ++i ){
			let entry = this.buffer[i];
			const haveStrings = entry.templateId && !entry.fileId && !entry.functionId;
			if( haveStrings )
				entries.push( entry );
			else{
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
	subscribe( applicationId:number|undefined, level:ELogLevel ){
		var subscription = { applicationId: applicationId, level: level, start:this.start, limit:this.view().limit };
		if( JSON.stringify(this.currentSubscription)!=JSON.stringify(subscription) ){
			this.buffer.length=0;
			this.data.clear();
			this.startIndexChange.next( 0 );
			//this.lengthChange.next( 0 );
			this.unsubscribe();
			this.level = level;
			this.currentSubscription = subscription;
			//this.subscription = this.appService.logs( subscription.applicationId, subscription.level, subscription.start, subscription.limit ).subscribe( traces => {this.onTrace(traces);} );
		}
	}

	unsubscribe(){
		if( this.subscription ){
			this.subscription.unsubscribe();
			this.subscription = undefined;
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

	onSort(sort: Sort|any){
		let sortedView = new LogView( this.view() );
		let newSort = sortedView.sort;
		let applySort =()=>{
			sortedView.sort = newSort;
			sortedView.type = ViewType.Adhoc;
			let newViews = [...this.views()];
			let index = newViews.findIndex( v => v.name==sortedView.name && v.type==sortedView.type );
			if( index==-1 ){
				newViews.push( sortedView );
				this.viewIndex.set( newViews.length - 1 );
				index = newViews.length - 1;
			}else
				newViews[index] = sortedView;

			this.views.set( [...this.views().filter( v => v.name!=sortedView.name || v.type!=sortedView.type ), sortedView] );
			this.viewIndex.set( this.views().length - 1 );
			this.data.clear();
			this.load();
		};
		if( !sort.direction ){
			if( sort.active=="time" )
				sort.direction = "asc";
			else{
				newSort.shift();
				applySort();
				return;
			}
		}
		let existingIndex = newSort.findIndex( s=>s.active==sort.active );
		if( existingIndex>0 ){
			newSort.splice( existingIndex, 1 );
			newSort.unshift( sort );
		}
		else if( existingIndex==0 )
			newSort[0] = sort;
		else
			newSort.unshift( sort );//column not yet in the sort list: make it primary (was a silent no-op)
		applySort();
	}
	onPagerChange( event:PageEvent ){
		if(	this.data.setPage(event.startIndex) )
 			this.load( this.data.allEntries.length );
	}
	onViewChange(index:number){
		this.views.set( this.views().filter(v=>v.type!=ViewType.Adhoc) );
		this.viewIndex.set( index );
		ProfileStore.setViewIndex( "logs", index );
		this.data.clear();
		this.load();
	}
	async onViewSave(view:LogView){
		if( (view.isSystem || view.isAdhoc) && !this.views().find(v=>v.name==view.name && v.isSystem) )
			view.type = ViewType.User;
		let newViews = this.views().filter( v=>v.type!=ViewType.Adhoc );
		let newIndex = newViews.findIndex( v=>v.name==view.name && view.type==v.type );
		if( newIndex==-1 ){
			newViews.push( view );
			newIndex = newViews.length - 1;
		}else
			newViews[newIndex] = view;
		this.views.set( newViews );
		this.viewIndex.set( newIndex );
		verify( view.type==ViewType.User );
		if( view.type==ViewType.User )
			this.profileStore.save( `logs/views`, newViews.filter(v=>v.isUser).map(v=>v.toJson(undefined)) );

		this.data.clear();
		this.load();
		this.isSettings.set( false );
	}
	onViewShow(view:LogView){
		this.data.clear();
		this.isSettings.set( false );
		if( view.name?.endsWith("*") && view.isAdhoc )
			view.name = view.name.substring( 0, view.name.length-1 );
		view.type = ViewType.Adhoc;
		let existing = this.views().findIndex( v=>v.name==view.name && view.type==v.type );
		let newView = new LogView( view );
		let newViews = [...this.views()];//new array + views.set so the signal notifies — in-place mutation only refreshed consumers when viewIndex happened to change
		let newIndex;
		if( existing>=0 ){
			newViews[existing] = newView;
			newIndex = existing;
		}
		else{
			newViews.push( newView );
			newIndex = newViews.length - 1;
		}
		this.views.set( newViews );
		this.viewIndex.set( newIndex );
		this.load();
	}
	onViewDelete(view:LogView){
		verify( view.type==ViewType.User );
		this.views.set( this.views().filter( v=>v.name!=view.name || v.type!=view.type ) );
		this.viewIndex.set( 0 );
		this.profileStore.save( `logs/views`, this.views().filter(v=>v.isUser).map(v=>v.toJson(undefined)) );
	}

	cellClick( entry:Entry ){
		let current =	this.selectedEntry;
		if( current != entry )
			entry.selected = true;
		if( current )
			current.selected = false;
	}
	hideSelectedMessage(){
		this.profile.level = ELogLevel.Information;
		this.profile.hiddenMessages.push( this.selectedEntry!.templateId );
		this.profile.level = ELogLevel.Debug;
		this.filterData();
	}
	clearHiddenMessages(){
		this.profile.hiddenMessages.length=0;
		this.filterData();
	}
	filterData(){
		if( this.data.filterData({messageIds: this.profile.hiddenMessages, message: this.filter, level: this.level}) )
			this.load( this.data.allEntries.length );
	}
	//navigate to next message with same template id
	async navigateNext(){
		if( !this.data.selectNext() ){
			await this.load( this.data.allEntries.length );
			if( !this.data.selectNext() )
				this.snackBar.warn( "No more instances found.", (m)=>console.log(m) );
		}
	}
	applyFilter( value:string ){
		this.filter = value;
		this.filterData();
	}
	get sort(){return this.view().sort;}
	service = input.required<IGraphQL>();
	profile!:LogSettings;
	data!: LogDataSource;
	get paused(){return this.data.paused;} set paused(value){this.data.paused=value;}
	connected = false;
	displayedColumns = computed( () => {
		return this.view().fields.filter(f=>f.displayed).map( (f)=>f.name );
	} );
	//configuration = { displayHeader:true }
	@ViewChild('mainTable',{static: false}) _table!:MatTable<Entry>;

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
	message(entry:Entry):string{
		return this.data.message(entry);
	}

	get applicationId(){ return this.profile.applicationId; } set applicationId(value){ this.profile.applicationId=value; }
	get columns():Record<string,string>{ return LogEntries.columns; }
	get start():Date{ return this._start.value; } set start(value:Date){ this._start.setValue(value); this.profile.start = value; } private _start = new FormControl();
	private filter!:string; 	//get filter(){return _filter;} set filter(value){ this._filter = value.trim().toLowerCase(); }
	startChange( event: MatDatepickerInputEvent<Date> ){ this.subscribe( this.applicationId, this.level ); }
	private buffer:Entry[] = [];
	static DefaultSubscription:ISubscription={ applicationId: 0, level:  ELogLevel.NoLog, start:null };
	private currentSubscription:ISubscription=Logs.DefaultSubscription;//actual subscribtion
	isLoading = signal<boolean>( true );
	isSettings = signal<boolean>( false );
//	lengthChange = new Subject<number>();
	startIndexChange = new Subject<number>();
	get level():ELogLevel{ return this.profile.level; } set level( value:ELogLevel ){ this.profile.level=value; }
	private get application():AppStatus|undefined{ return this.applications.find( (existing)=>{return existing.id==this.applicationId;} ); }
	applications:AppStatus[]=[];
	schema:TableSchema = LogView.schema;
	private subscription:Unsubscribable|undefined;
	//private applicationStrings:ApplicationStrings = new ApplicationStrings();
	//private pushTimeout:{ entries: TraceEntry[], id:any, end:number };
	//get selectedIndex(){ return this.selectedEntry?.index; } set selectedIndex(x){ this.selectedEntry = this.data.entries.find( (y)=>y.index==x ); }
	get selectedEntry(){ return this.data.allEntries.find( (e)=>e.selected ); }
	views = signal<LogView[]>(null as any);
	view = computed<LogView>( () => this.views()[this.viewIndex()] );
	get viewCopy(){ return new LogView( this.view() ); }
	viewIndex = signal<number>(null as any);
	profileStore = inject(ProfileStore);
}

interface ISubscription{ applicationId:number|undefined, level:ELogLevel, start:Date|null }