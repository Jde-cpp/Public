import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject } from '@angular/core';
import { MatTabsModule } from '@angular/material/tabs';
import { ActivatedRoute } from '@angular/router';
import { ComponentPageTitle, ProfileStore } from 'jde-spa';
import { AppService, LogDetail, LogSettingsPanel, QLList, QLListData, TableSettings } from 'jde-framework';
import { RouteItem } from 'jde-spa';
import { GatewayService } from '../../services/gateway-service';
import { Gateway } from '../../services/gateway-service';

@Component( {
		styleUrls: ['gateway-detail.scss'],
		templateUrl: './gateway-detail.html',
		host: {class:'main-content mat-drawer-container my-content'},
		imports: [MatTabsModule, QLList, LogDetail, LogSettingsPanel]
})
export class GatewayDetail implements OnInit, OnDestroy{
	private route:ActivatedRoute = inject( ActivatedRoute );
	private componentPageTitle:ComponentPageTitle = inject( ComponentPageTitle );

	ngOnInit(): void {
		this.route.data.subscribe( async (routeData)=>{
			this.pageData = <QLListData>routeData["data"];
			this.sideNav.set( this.pageData.routing );
			const instanceName = this.pageData.routing.path.split('/').slice(-1)[0];
			this.componentPageTitle.title = `${instanceName} - Gateway`;
			this.gateway = await this.gatewayService.gateway( instanceName );
			this.isLoading.set( false );
			this.instanceId.set( await this.appService.instancePK(instanceName, "OpcGateway") );
		});
	}
	ngOnDestroy(): void {
		ProfileStore.setTabIndex( 'gateway-detail', this.tabIndex );
		// Cleanup logic if needed
	}

	tabIndexChanged( index:number ){ this.tabIndex = index;}

	pageData!:QLListData;
	get connections(){ return this.pageData?.results["serverConnections"]; }
	tabIndex:number = ProfileStore.tabIndex( 'gateway-detail' );
	sideNav = model.required<RouteItem>();

	gateway!:Gateway;
	gatewayService = inject(GatewayService);
	appService = inject(AppService);
	instanceId = signal<number|undefined>( undefined );
	isLoading = signal<boolean>( true );
}

export const gatewayTableSettings:TableSettings = {
	columns: ["name", "certificateUri", "url", {name:"opcSessions", displayName:"Sessions", selection:"count"}, {name:"opcConnections", displayName:"Connections", selection:"count"}, "description" ],
	sort: "name"
}