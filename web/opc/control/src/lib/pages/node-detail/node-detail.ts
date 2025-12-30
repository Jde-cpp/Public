import { Component, computed, model, OnDestroy, OnInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTabsModule } from '@angular/material/tabs';
import { NodePageData } from '../../services/resolvers/node.resolver';
import { ActivatedRoute } from '@angular/router';
import { ComponentPageTitle, DocItem } from 'jde-spa';
import { Server } from '../../model/Server';
import { NodeRoute } from '../../model/NodeRoute';
import { NodeChildren } from './node-children/node-children';
import { NodeRoles } from './node-roles/node-roles';
import { UaNode } from '../../model/Node';
import { ServerProperties } from './server-properties/server-properties';

@Component( {
	templateUrl: './node-detail.html',
	styleUrls: ['./node-detail.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule,MatTabsModule, NodeChildren, NodeRoles, ServerProperties]
})
export class NodeDetail implements OnDestroy, OnInit{
	constructor(
		private route: ActivatedRoute, private componentPageTitle:ComponentPageTitle,
	){}
	ngOnDestroy(){
		//this.profile.save();
	}
	ngOnInit(){
		this.route.data.subscribe( (data)=>{
			this.pageData.set( data["pageData"] );
			this.sideNav.set( this.pageData().route );
			this.componentPageTitle.title = this.server().connection.name + (this.node().id==85 ? '' : `/${this.node().name}`);
		});
	}

	node = computed( ()=>this.sideNav().node );
	pageData = signal<NodePageData>( null );
	get profile(){ return this.pageData().route.profile; }
	server = computed( ()=>this.pageData().server );
	sideNav = model.required<NodeRoute>();
	tabIndexChanged( index:number ){ this.profile.tabIndex = index; }
}
