import { Component, effect, OnInit, OnDestroy, Inject, signal, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ActivatedRoute, Router } from '@angular/router';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { ComponentPageTitle } from 'jde-spa';
import { DetailResolverData, IErrorService, IGraphQL, IProfile, Properties} from 'jde-framework';

import { ServerCnnctn } from '../../model/ServerCnnctn';
import { Gateway, GatewayService } from '../../services/gateway.service';

@Component( {
	selector: 'roles',
	templateUrl: './client-detail.html',
	styleUrls: ['./client-detail.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties]
})
export class GatewayDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private snackbar: IErrorService ){
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
		this.profile.save();
	}
	async ngOnInit(){
		const segments = this.router.url.split( "/" );
		this.gateway = await this.gatewayService.gateway( segments[segments.length-2] );
		this.sideNav.set( this.pageData.routing );
	}
	tabIndexChanged( index:number ){ this.profile.value.tabIndex = index;}

	async onSubmitClick(){
		try{
			const upsert = new ServerCnnctn( {
//				id:this.properties().id,
				...this.properties(),
			});
			const mutation = upsert.mutation( this.serverCnnctn );
			await this.gateway.mutation( mutation, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.exceptionInfo( e, "Save failed.", (m)=>console.log(m) );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}

	serverCnnctn:ServerCnnctn;
	pageData:DetailResolverData<ServerCnnctn>;
	ctor:new (item: any) => any = ServerCnnctn;
	isChanged = signal<boolean>( false );
	get profile(){ return this.pageData.pageSettings.profile;}

	properties = signal<ServerCnnctn>( null );
	get schema(){ return this.pageData.schema; }
	sideNav = signal<any>( null );
	gatewayService:GatewayService = inject( GatewayService );
	gateway:Gateway;
}