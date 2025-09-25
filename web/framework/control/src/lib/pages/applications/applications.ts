import {Component, OnDestroy, OnInit, Inject} from '@angular/core';
import { MatDialog, MatDialogRef, MAT_DIALOG_DATA } from '@angular/material/dialog';
import { SeverityPicker } from '../../shared/severity-picker/severity-picker'
import { DomSanitizer } from "@angular/platform-browser";
import {Params} from '@angular/router';
import { MatIcon, MatIconRegistry } from "@angular/material/icon";
import { ComponentPageTitle } from 'jde-spa';
import {AppService} from '../../services/app/app.service';
import {Observable, Subscription} from 'rxjs';
import {App, AppStatus} from '../../services/app/application';
import {IErrorService} from '../../services/error/IErrorService';
import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import { MatCardModule } from '@angular/material/card';
import { MatTableModule } from '@angular/material/table';

@Component({
    selector: 'applications',
    templateUrl: './applications.html',
    styleUrls: ['./applications.scss'],
    imports: [MatCardModule, MatIcon, MatTableModule]
})
export class Applications implements OnInit, OnDestroy
{
	params: Observable<Params>;
	routeParamSubscription: Subscription;
	_categoryListSummary: string;

	constructor( public _componentPageTitle: ComponentPageTitle, private appService:AppService, private dialog:MatDialog, private matIconRegistry: MatIconRegistry, private domSanitizer: DomSanitizer, @Inject('IErrorService') private cnsl: IErrorService ){
		const url = this.domSanitizer.bypassSecurityTrustResourceUrl("../assets/download.svg");
		this.matIconRegistry.addSvgIcon( "ib", "download.svg" );
	}

	async ngOnInit(){
		this._componentPageTitle.title = "Applications";
		this.applications.length = 0;
		try{
			//await this.appService.custom( 99, new TextEncoder().encode("adsf") );
			const applications = await this.appService.queryArray<App>( "apps{id name dbLogLevel fileLogLevel}" );
			for( let app of applications )
				this.applications.push( new AppStatus(app) );
			this.subscription = this.appService.statuses();
			this.subscription.subscribe( apps => {this.onStatus(apps);} );
		}
		catch( e ){
			this.cnsl.exceptionInfo( e, "Could not load applications", (m)=>console.log(m) );
		}
	}

	onStatus = ( status:FromServer.IStatus ):void =>{
	this.mainOn = true;
		let existing = this.applications.find( app=>app.id==status.applicationId );
		if( existing )
			existing.status = status;
		else
		console.warn( `could not find app ${status.applicationId}` );
	}

	ngOnDestroy(){
		this.appService.statusUnsubscribe( this.subscription );
	}

	togglePower( app:AppStatus ){
		if( app.on )
			this.appService.mutation( `stopApplicationInstance(id:${app.instanceId})` );
		else
			this.cnsl.error( "applicationService.start failed", null );
	}

/*	edit( app:AppStatus ){
		const dialogRef = this.dialog.open(EditDialog, {
			width: '600px',
			data: {app: app}
		});
	}*/
	applications:AppStatus[]=[];
	mainOn:boolean=false;
	private subscription:Observable<FromServer.IStatus>;
}

// interface DialogData{
// 	app: Application;
// }

// interface LogOption{
// 	name:string;
// 	value:FromServer.ELogLevel;
// }

// @Component( { selector: 'missing-dates-dialog',	templateUrl: 'edit-dialog.html'} )
// export class EditDialog{
// 	constructor( public dialogRef:MatDialogRef<EditDialog>, @Inject(MAT_DIALOG_DATA) public data:DialogData, private appService:AppService ){
// 		this.dbLevel = this.app.status?.dbLogLevel;
// 		this.clientLevel = this.app.status?.fileLogLevel;
// 	}

// 	onCancelClick(): void{
// 	  this.dialogRef.close();
// 	}

// 	onSaveClick():void{
// 		this.saving = true;
// 		this.appService.updateLogLevel( this.app.instanceId, this.clientLevel, this.dbLevel );
// 		this.dialogRef.close();
// 	}
// 	dbLevel:FromServer.ELogLevel;
// 	clientLevel:FromServer.ELogLevel;
// 	options:LogOption[]=[{name:'Trace',value:FromServer.ELogLevel.Trace},{name:'Debug',value:FromServer.ELogLevel.Debug}, {name:'Info',value:FromServer.ELogLevel.Information},{name:'Warning',value:FromServer.ELogLevel.Warning},{name:'Error',value:FromServer.ELogLevel.Error},{name:'Critical',value:FromServer.ELogLevel.Critical},{name:'None',value:FromServer.ELogLevel.None}]
// 	get app():Application{ return this.data.app; }
// 	saving:boolean=false;
// }