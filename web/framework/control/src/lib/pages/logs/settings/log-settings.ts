import { CommonModule } from '@angular/common';
import { Component, OnDestroy, OnInit, ViewChild, input, output, signal } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatTabsModule } from '@angular/material/tabs';
import { MatToolbar } from '@angular/material/toolbar';
import { ProfileStore } from 'jde-spa';
import { LogTags } from '../tags/log-tags';
import { IGraphQL } from '../../../services/IGraphQL';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { Mutation, MutationType } from '../../../model/ql/Mutation';

type TagLevels = Record<string,string>;

@Component({
	selector: 'log-settings',
	templateUrl: './log-settings.html',
	styleUrls: ['./log-settings.scss'],
	imports: [CommonModule, MatButtonModule, MatTabsModule, MatToolbar, LogTags]
})
export class LogSettingsPanel implements OnInit, OnDestroy{
	constructor( private snackBar: SnackbarService ){}

	async ngOnInit(){ await this.load(); }
	ngOnDestroy(){ ProfileStore.setTabIndex( 'log-settings', this.tabIndex() ); }

	async load(){
		this.isLoading.set( true );
		this.error.set( null );
		try{
			const log = (m:string)=>console.log(m);
			const [setting, levels] = await Promise.all( [
				this.service().querySingle<{tags:Record<string,number>}>( "logSetting{tags}", null, log ),
				this.service().querySingle<{ [type:string]:TagLevels }>( `instanceTagLevel( id:${this.instanceId()} ){ text binary appServer }`, null, log )
			] );
			this.catalogue.set( ["default", ...Object.keys(setting?.tags ?? {}).sort()] );
			this.snapshot = { text: levels?.["text"] ?? {}, binary: levels?.["binary"] ?? {}, appServer: levels?.["appServer"] ?? {} };
			this.isLoading.set( false );
		}
		catch( e ){
			this.error.set( `${e}` );//without this the panel stays behind isLoading and renders blank
			this.isLoading.set( false );
			this.snackBar.exception( e, (m)=>console.log(m) );
		}
	}
	async save(){
		try{
			const args:any = {};
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
				if( Object.keys(diff).length )
					args[type] = diff;
			}
			if( Object.keys(args).length ){
				await this.service().mutate( new Mutation('instanceTagLevel', this.instanceId(), args, MutationType.Update), (m)=>console.log(m) );
				this.snapshot = { text: this.text.entries(), binary: this.binary.entries(), appServer: this.remote.entries() };
			}
			this.onSave.emit();
		}
		catch( e ){
			this.snackBar.exception( e, (m)=>console.log(m) );
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
	error = signal<string|null>( null );
	isLoading = signal<boolean>( true );
	tabIndex = signal<number>( ProfileStore.tabIndex('log-settings') );

	@ViewChild('binary') binary!: LogTags;
	@ViewChild('remote') remote!: LogTags;
	@ViewChild('text') text!: LogTags;
}
