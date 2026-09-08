import { Component, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { DetailPage, Properties} from 'jde-framework';

import { ServerProperties } from './server-properties/server-properties';
import { ServerCnnctn, ServerCnnctnProps } from '../../../model/server-cnnctn';
import { Gateway, GatewayService } from '../../../services/gateway-service';
import { Server } from '../../../model/server';

@Component( {
	templateUrl: './client-detail.html',
	styleUrls: ['./client-detail.scss'],
	//the trailing class is load-bearing:  Angular hashes a component's *shape* into its style-encapsulation id and leaves
		//the class name out, so four routed pages that now share DetailPage and this host string could collide with NG0912.
		host: {class:'main-content mat-drawer-container my-content client-detail'},
	imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, ServerProperties],
})
export class ClientDetail extends DetailPage<ServerCnnctn>{
	constructor(){ super( 'client-detail' ); }

	override async ngOnInit(){
		super.ngOnInit();
		const segments = this.router.url.split( "/" );
		this.gateway = await this.gatewayService.gateway( segments[segments.length-2] );
	}

	protected override get ctor(){ return ServerCnnctn; }
	protected override onRow(){}//no child collections - the connection is its properties
	protected override upsert():ServerCnnctn{
		return new ServerCnnctn( {
			...this.properties(),
		} as ServerCnnctnProps);
	}
	protected override get title(){ return this.row.name ? `${this.row.name} - Connection` : "New Connection"; }
	override get ql(){ return this.gateway; }//per-gateway, not a single injected service - resolved in ngOnInit

	async onDeleteClick(){
		const restore = this.isDeleted;
		try{
			await this.gateway.mutate( `${restore ? "restore" : "delete"}${this.row.type}(id:${this.row.id})`, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch( e ){
			this.snackbar.exception( `${restore ? "Restore" : "Delete"} failed.`, e );
		}
	}

	//what a user reads first, then the connection, then the two technical fields - the alphabetical default buried Description between them and put Url last
	readonly fieldOrder = ["target", "name", "description", "url", "certificateUri", "defaultBrowseNs"];
	readonly fieldLabels = { url: "URL", certificateUri: "Certificate URI", defaultBrowseNs: "Default Namespace" };//the camelCase split gives "Url", "Certificate Uri", "Default Browse Ns"
	get serverCnnctn(){ return this.row; }//the template's name for it
	get isDeleted():boolean{ return this.row?.deleted!=null; }//only populated when show-deleted is on - the query drops the column otherwise
	//Gates the Connection tab as well as the Id field.  The tab used to be gated on `server`, so an unreachable server had
	//no tab at all;  it now shows its not-connected state, and the base's `!id` clamp (review3 L2) covers the one case left.
	get isNew():boolean{ return !this.row?.id; }//the Id field: target is the connection's identity and the gateway meta refuses to update it, so the form does not offer it
	get server(): Server|undefined{ return this.row?.server; }
	get serverError(): string|undefined{ return this.row?.serverError; }
	gatewayService:GatewayService = inject( GatewayService );
	gateway!:Gateway;
}
