import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject } from '@angular/core';
import { MatTabsModule } from '@angular/material/tabs';
import { ActivatedRoute } from '@angular/router';
import {  ProfileStore } from 'jde-spa';
import { AppService, Logs, QLList, QLListData } from 'jde-framework';
import { RouteItem } from 'jde-spa';
import { GatewayService } from 'jde-opc';
import { Gateway } from '../../services/gateway.service';

@Component( {
		styleUrls: ['gateway-detail.scss'],
		templateUrl: './gateway-detail.html',
		host: {class:'main-content mat-drawer-container my-content'},
		imports: [MatTabsModule, QLList, Logs]
})
export class GatewayDetail implements OnInit{
	constructor( private route: ActivatedRoute )
	{}

	ngOnInit(): void {
		this.route.data.subscribe( async (routeData)=>{
			this.pageData = <QLListData>routeData["data"];
			this.sideNav.set( this.pageData.routing );
			this.gateway = await this.gatewayService.gateway( this.pageData.routing.path.split('/').slice(-1)[0] );
		});
	}

	tabIndexChanged( index:number ){ this.tabIndex = index;}

	pageData:QLListData;
	get connections(){ return this.pageData?.results["serverConnections"]; }
	tabIndex:number = ProfileStore.tabIndex( 'gateway-detail' );
	sideNav = model.required<RouteItem>();

	gateway:Gateway;
	gatewayService = inject(GatewayService);
}