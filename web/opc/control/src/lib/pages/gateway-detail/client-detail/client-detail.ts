import { Component, effect, OnInit, OnDestroy, signal, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ActivatedRoute, Router } from '@angular/router';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { ProfileStore } from 'jde-spa';
import { DetailResolverData, SnackbarService, IGraphQL, Properties} from 'jde-framework';

import { ServerProperties } from './server-properties/server-properties';
import { ServerCnnctn, ServerCnnctnProps } from '../../../model/server-cnnctn';
import { Gateway, GatewayService } from '../../../services/gateway-service';
import { Server } from '../../../model/server';

@Component( {
	templateUrl: './client-detail.html',
	styleUrls: ['./client-detail.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, ServerProperties],
})
export class ClientDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private snackbar: SnackbarService ){
		effect(() => {
			if( !this.properties() )
				return;
			if( !this.properties().canSave )
				this.isChanged.set( false );
			else if( !this.properties().equals(this.serverCnnctn.properties) )
				this.isChanged.set( true );
		});
		route.data.subscribe( (data)=>{
			this.pageData = data["pageData"];
			this.serverCnnctn = new ServerCnnctn( this.pageData.row );
			this.pageData.row = null;

			this.properties.set( this.serverCnnctn.properties );
		});
	}
	ngOnDestroy(){
		ProfileStore.setTabIndex( 'client-detail', this.tabIndex );
	}
	async ngOnInit(){
		const segments = this.router.url.split( "/" );
		this.gateway = await this.gatewayService.gateway( segments[segments.length-2] );
		this.sideNav.set( this.pageData.routing );
	}
	tabIndexChanged( index:number ){ this.tabIndex = index;}

	async onSubmitClick(){
		try{
			const upsert = new ServerCnnctn( {
//				id:this.properties().id,
				...this.properties(),
			} as ServerCnnctnProps);
			const mutation = upsert.mutation( this.serverCnnctn );
			await this.gateway.mutate( mutation, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.exception( "Save failed.", e );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}

	async onDeleteClick(){
		const restore = this.isDeleted;
		try{
			await this.gateway.mutate( `${restore ? "restore" : "delete"}${this.serverCnnctn.type}(id:${this.serverCnnctn.id})`, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch( e ){
			this.snackbar.exception( `${restore ? "Restore" : "Delete"} failed.`, e );
		}
	}

	serverCnnctn!:ServerCnnctn;
	pageData!:DetailResolverData<ServerCnnctn>;
	ctor:new (item: any) => any = ServerCnnctn;
	isChanged = signal<boolean>( false );

	properties = signal<ServerCnnctn>( null as any );
	get isDeleted():boolean{ return this.serverCnnctn?.deleted!=null; }//only populated when show-deleted is on - the query drops the column otherwise
	get isNew():boolean{ return !this.serverCnnctn?.id; }
	get schema(){ return this.pageData.schema; }
	get server(): Server{ return this.serverCnnctn?.server; }
	sideNav = signal<any>( null );
	tabIndex:number = ProfileStore.tabIndex( 'client-detail' );
	gatewayService:GatewayService = inject( GatewayService );
	gateway!:Gateway;
}