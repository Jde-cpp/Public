import { Component, OnInit, OnDestroy, Inject, ViewChild, input, signal, model, computed, Injectable, inject } from '@angular/core';
import { MatTabsModule } from '@angular/material/tabs';
import { ActivatedRoute } from '@angular/router';
import { DetailResolverData, LocalProfileStore, QLList, QLListData } from 'jde-framework';
import { DocItem } from 'jde-spa';

@Component( {
		styleUrls: ['gateway-detail.scss'],
		templateUrl: './gateway-detail.html',
		host: {class:'main-content mat-drawer-container my-content'},
		imports: [MatTabsModule, QLList]
})
export class GatewayDetail implements OnInit{
	constructor( private route: ActivatedRoute )
	{}

	ngOnInit(): void {
		this.route.data.subscribe( (routeData)=>{
			this.pageData = <QLListData>routeData["data"];
			//this.sideNav.set( this.pageData.routing );
		});
	}

	tabIndexChanged( index:number ){ this.tabIndex = index;}

	pageData:QLListData;
	get connections(){ return this.pageData?.data["serverConnections"];}
	tabIndex:number = LocalProfileStore.tabIndex( 'gateway-detail' );
	sideNav = model.required<DocItem>();
}
