import { Component, computed, Inject, model, OnDestroy, OnInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTabsModule } from '@angular/material/tabs';
import { NodePageData } from '../../services/resolvers/node.resolver';
import { ActivatedRoute } from '@angular/router';
import { ComponentPageTitle, DocItem } from 'jde-spa';
import { NodeRoute } from '../../model/NodeRoute';
import { NodeChildren } from './node-children/node-children';
import { UaNode } from '../../model/Node';
import { NodeAccess } from './node-access/node-access';
import { IProfileStore } from '../../../../../jde-framework/src/lib/services/profile/profile.store';
import { LocalProfileStore } from 'jde-framework';

@Component( {
	templateUrl: './node-detail.html',
	styleUrls: ['./node-detail.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatTabsModule, NodeChildren, NodeAccess]
})
export class NodeDetail implements OnDestroy, OnInit{
	constructor( private activatedRoute: ActivatedRoute, private componentPageTitle:ComponentPageTitle, @Inject('IProfileStore') private profile: IProfileStore )
	{}
	ngOnDestroy(){
		LocalProfileStore.setTabIndex( `nodeDetail/${JSON.stringify(this.node().toJson())}`, this.tabIndex );
  }
	ngOnInit(){
		this.activatedRoute.data.subscribe( (data:any)=>{
			this.pageData.set( data["pageData"] );
			this.sideNav.set( this.pageData().route );
			this.componentPageTitle.title = this.server().connection.name + (this.node().id==85 ? '' : `/${this.node().name}`);
		});
	}

	node = computed( ()=>this.sideNav().node );
	pageData = signal<NodePageData>( null );
	//get profile(){ return this.pageData().route.profile; }
	server = computed( ()=>this.pageData().server );
	sideNav = model.required<NodeRoute>();
	tabIndex:number;
	onTabIndexChanged( index:number ){ this.tabIndex = index; }
}
