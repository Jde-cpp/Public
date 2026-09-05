import { CommonModule } from '@angular/common';
import { Component, OnDestroy, OnInit, ViewChild, computed, input, output, signal, inject } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatTabsModule } from '@angular/material/tabs';
import { MatToolbar } from '@angular/material/toolbar';
import { IENVIRONMENT, IEnvironment, ProfileStore } from 'jde-spa';
import { LogEntries } from '../log-entry';
import { LogTags } from '../tags/log-tags';
import { IGraphQL } from '../../../services/graphql';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { errorMessage } from '../../../utils/errors';
import { Mutation, MutationType } from '../../../model/ql/mutation';

type TagLevels = Record<string,string>;
//instanceTagLevel answers with the tags grouped under their level: { "Debug":["sql",["socket","client","read"]] }.  A
//combined tag has no name to be an object key by, so it arrives as the array Jde::ToValue produced; tagSeparator is how
//the server spells one as a name (Jde::ToLogTags splits on it), and that is what the editor and the save diff key off.
type LevelTags = Record<string,(string|string[])[]>;
const tagSeparator = ".";//Jde::TagSeparator, fwk log/logTags.h - config keys and instance rows are spelled with it too.
//every catalogue is ordered on the name the row shows, not the wire spelling — tagName reorders a combined tag's
//parts, so "read.server.socket" shows as "Socket.Server.Read" and a bare sort() files it under R.
const byName = ( a:string, b:string )=>LogEntries.tagName( a ).localeCompare( LogEntries.tagName(b) );
const flatten = ( group:LevelTags|undefined ):TagLevels=>{
	const y:TagLevels = {};
	for( const [level, tags] of Object.entries(group ?? {}) )
		for( const tag of tags ?? [] )
			y[Array.isArray(tag) ? tag.join(tagSeparator) : tag] = level;
	return y;
};

@Component({
	selector: 'log-settings',
	templateUrl: './log-settings-panel.html',
	styleUrls: ['./log-settings-panel.scss'],
	imports: [CommonModule, MatButtonModule, MatTabsModule, MatToolbar, LogTags]
})
export class LogSettingsPanel implements OnInit, OnDestroy{
	private snackBar:SnackbarService = inject( SnackbarService );
	private environment:IEnvironment = inject( IENVIRONMENT );

	async ngOnInit(){ await this.load(); }
	ngOnDestroy(){ ProfileStore.setTabIndex( 'log-settings', this.tabIndex() ); }

	async load(){
		this.isLoading.set( true );
		this.error.set( null );
		try{
			const log = (m:string)=>console.log(m);
			const [setting, levels] = await Promise.all( [
				this.service().querySingle<{tags:Record<string,number>}>( "logSetting{tags}", null, log ),
				this.service().querySingle<{ [type:string]:LevelTags }>( `instanceTagLevel( id:${this.instanceId()} ){ text binary appServer }`, null, log )
			] );
			this.catalogue.set( ["default", ...Object.keys(setting?.tags ?? {}).sort(byName)] );
			this.snapshot = { text: flatten(levels?.["text"]), binary: flatten(levels?.["binary"]), appServer: flatten(levels?.["appServer"]) };
			this.isLoading.set( false );
		}
		catch( e ){
			this.error.set( errorMessage(e, "Could not load log settings.") );//errorMessage, not `${e}`: an HttpErrorResponse and a {error:IError} rejection both render "[object Object]".  Without this the panel stays behind isLoading and renders blank
			this.isLoading.set( false );
			this.snackBar.exception( "Could not load log settings.", e );
		}
	}
	async save(){
		try{
			const args:Record<string,unknown> = {};
			for( const [type, child] of <[string,LogTags][]>[ ["text", this.text], ["binary", this.binary], ["appServer", this.remote] ] ){
				const current = child.entries();
				const previous = this.snapshot[type];
				const diff:Record<string,string|null> = {};
				for( const [tag,level] of Object.entries(current) )
					if( previous[tag]!=level )
						diff[tag] = level;
				for( const tag of Object.keys(previous) )
					if( !(tag in current) )
						diff[tag] = null;
				//a record per override - a combined tag is an array, which is no object key.
				if( Object.keys(diff).length )
					args[type] = Object.entries(diff).map( ([tag,level])=>({ tags: tag.split(tagSeparator), level }) );
			}
			if( Object.keys(args).length ){
				await this.service().mutate( new Mutation('instanceTagLevel', this.instanceId(), args, MutationType.Update), (m)=>console.log(m) );
				this.snapshot = { text: this.text.entries(), binary: this.binary.entries(), appServer: this.remote.entries() };
			}
			this.onSave.emit();
		}
		catch( e ){
			this.snackBar.exception( "Could not save log settings.", e );
		}
	}
	async cancel(){
		await this.load();
		this.onCancel.emit();
	}

	service = input.required<IGraphQL>();
	instanceId = input.required<number>();
	onSave = output<void>();
	onCancel = output<void>();

	snapshot:{ [type:string]:TagLevels } = { text:{}, binary:{}, appServer:{} };
	catalogue = signal<string[]>( [] );
	//break maps to _breakLevel (fwk src/log/break.cpp), which only fires with a debugger attached, so it is offered
	//on the text sink alone - the one that shares the local process - and never on a production build.
	textCatalogue = computed<string[]>( ()=>{
		const y = this.catalogue();
		if( this.environment.get<boolean>("production") || !y.length || y.includes(LogSettingsPanel.breakTag) )
			return y;
		return [ y[0], ...y.slice(1).concat(LogSettingsPanel.breakTag).sort(byName) ];//load() heads the catalogue with 'default', which is not a tag and stays first
	});
	error = signal<string|null>( null );
	isLoading = signal<boolean>( true );
	tabIndex = signal<number>( ProfileStore.tabIndex('log-settings') );
	static breakTag = "break";

	@ViewChild('binary') binary!: LogTags;
	@ViewChild('remote') remote!: LogTags;
	@ViewChild('text') text!: LogTags;
}
