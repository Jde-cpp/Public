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
	//not the base's `!id`: this page's one extra tab is gated on `server`, so an EXISTING connection with none has only
	//the Properties tab too, and a stored index past it would hard-loop mat-tab-group the same way (review3 L2).
	protected override get onlyPropertiesTab(){ return !this.server; }
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

	get serverCnnctn(){ return this.row; }//the template's name for it
	get isDeleted():boolean{ return this.row?.deleted!=null; }//only populated when show-deleted is on - the query drops the column otherwise
	get isNew():boolean{ return !this.row?.id; }
	get server(): Server{ return this.row?.server; }
	gatewayService:GatewayService = inject( GatewayService );
	gateway!:Gateway;
}
