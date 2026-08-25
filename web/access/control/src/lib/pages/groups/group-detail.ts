import { Component, effect, OnInit, OnDestroy, signal, inject, model } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { CommonModule } from '@angular/common';
import { ActivatedRoute, Router } from '@angular/router';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { ComponentPageTitle, RouteItem, ProfileStore } from 'jde-spa';
import { arraysEqual, cloneClassArray, DetailResolverData, Properties, SnackbarService, IGraphQL, QLSelector, Style, toIdArray, TargetRow} from 'jde-framework';

import { RolePK } from '../../model/role';
import { PermissionTable } from '../../shared/permissions/permission-table';
import { Permission } from '../../model/permission';
import { AccessService } from '../../services/access-service';
import { Group, GroupPK } from '../../model/group';
import { UserPK } from '../../model/user';

@Component( {
    templateUrl: './group-detail.html',
		styleUrls: ['./group-detail.scss'],
		host: {class:'main-content mat-drawer-container my-content'},
    imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, PermissionTable, QLSelector]
})
export class GroupDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, private snackbar: SnackbarService ){
		effect(() => {
			if( !this.properties() )
				return;
			if( !this.properties().canSave )
				this.isChanged.set( false );
			else if( !(this.properties() as Group).equals(this.group.properties) )
				this.isChanged.set( true );
		});

		effect(() => {
			if( this.users() && !arraysEqual(TargetRow.idArray(this.group.users ?? []),this.users().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.childGroups() && !arraysEqual(TargetRow.idArray(this.group.childGroups ?? []),this.childGroups().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.roles() && !arraysEqual(TargetRow.idArray(this.group.roles), this.roles().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.permissions() && !Permission.arraysEqual(this.group?.permissions ?? [], this.permissions()) )//value-compare like role-detail — identity arraysEqual on the clones marked the page dirty on load
				this.isChanged.set( true );
		});

		route.data.subscribe( (data)=>{
			this.pageData = data["pageData"];
			this.group = new Group( this.pageData.row );
			if( !this.group.id && this.tabIndex()>0 )
				this.tabIndex.set( 0 ); //only the Properties tab exists on $new - a stored index past it hard-loops mat-tab-group.
			this.pageData.row = null;
			this.properties.set( this.group.properties );
			this.users.set( new SelectionModel<UserPK>(true, TargetRow.idArray(this.group.users)) );
			this.childGroups.set( new SelectionModel<GroupPK>(true, TargetRow.idArray(this.group.childGroups)) );
			this.permissions.set( cloneClassArray(this.group.permissions, Permission) );
			this.roles.set( new SelectionModel<RolePK>(true, TargetRow.idArray(this.group.roles)) );
			this.componentPageTitle.title = this.group.name;
		});
	}
	ngOnDestroy(){
		ProfileStore.setTabIndex( 'groupDetail', this.tabIndex() );
	}
	ngOnInit(){
		this.sideNav.set( this.pageData.routing );
	}
	onTabIndexChanged( index:number ){ this.tabIndex.set(index); }

	async onSubmitClick(){
		try{
			const upsert = new Group( {id:this.properties().id, ...this.properties(), permissions: this.permissions(), users: this.users().selected, roles: this.roles().selected, childGroups: toIdArray(this.childGroups().selected)} );
			const mutation = upsert.mutation( this.group );
			await this.ql.mutate( mutation, (m)=>console.log(m) );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.exception( "Save failed.", e );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}

	group!:Group;
	get id(){ return this.group.id; }
	ctor:new (item: any) => any = Group;
	isChanged = signal<boolean>( false );
	properties = signal<Partial<Group>>( null as any );
	users = signal<SelectionModel<UserPK>>( null as any );
	childGroups = signal<SelectionModel<GroupPK>>( null as any );
	roles = signal<SelectionModel<RolePK>>( null as any );
	permissions = signal<Permission[]>( null as any );
	pageData!:DetailResolverData<Group>;
	get schema(){ return this.pageData.schema; }
	sideNav = model.required<RouteItem>();
	tabIndex = signal<number>( ProfileStore.tabIndex('groupDetail') );
	excludedColumns = [...groupTableSettings.excludedColumns, "provider"];//a group has no login provider - the column exists only because groups share the identity view with users.
	ql:IGraphQL = inject( AccessService );
}
export const groupTableSettings = {
	excludedColumns: ["isGroup", "members"],
	columns: [ //without this the list falls back to ListRoute's name/created/updated/deleted/target default.
		{ name:"name", style: new Style(300) },
		"target",
		"description"
	],
	collectionName: "groups"
}