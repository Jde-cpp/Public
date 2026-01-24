import { Component, effect, OnInit, OnDestroy, signal, inject, Inject, model } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { CommonModule } from '@angular/common';
import { ActivatedRoute, Router } from '@angular/router';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { ComponentPageTitle, DocItem } from 'jde-spa';
import { arraysEqual, cloneClassArray, DetailResolverData, Properties, IErrorService, IGraphQL, QLSelector, toIdArray, TargetRow, LocalProfileStore} from 'jde-framework';

import { RolePK } from '../../model/Role';
import { PermissionTable } from '../../shared/permissions/permission-table';
import { Permission } from '../../model/Permission';
import { AccessService } from '../../services/access.service';
import { Group, GroupPK } from '../../model/Group';
import { User, UserPK } from '../../model/User';

@Component( {
    templateUrl: './group-detail.html',
		styleUrls: ['./group-detail.scss'],
		host: {class:'main-content mat-drawer-container my-content'},
    imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, PermissionTable, QLSelector]
})
export class GroupDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IErrorService') private snackbar: IErrorService ){
		effect(() => {
			if( !this.properties() )
				return;
			if( !this.properties().canSave )
				this.isChanged.set( false );
			else if(  !this.properties().equals(this.group.properties) )
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
			if( !arraysEqual(this?.group?.permissions, this.permissions()) )
				this.isChanged.set( true );
		});

		route.data.subscribe( (data)=>{
			this.pageData = data["pageData"];
			this.group = new Group( this.pageData.row );
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
		LocalProfileStore.setTabIndex( 'groupDetail', this.tabIndex() );
	}
	async ngOnInit(){
		this.sideNav.set( this.pageData.routing );
	}
	onTabIndexChanged( index:number ){ this.tabIndex.set(index); }

	async onSubmitClick(){
		try{
			const upsert = new Group( {id:this.properties().id, ...this.properties(), permissions: this.permissions(), users: this.users().selected, roles: this.roles().selected, childGroups: toIdArray(this.childGroups().selected)} );
			const mutation = upsert.mutation( this.group );
			await this.ql.mutate( mutation );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.exceptionInfo( e, "Save failed.", (m)=>console.log(m) );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}

	group:Group;
	get id(){ return this.group.id; }
	ctor:new (item: any) => any = User;
	isChanged = signal<boolean>( false );
	properties = signal<Group>( null );
	users = signal<SelectionModel<UserPK>>( null );
	childGroups = signal<SelectionModel<GroupPK>>( null );
	roles = signal<SelectionModel<RolePK>>( null );
	permissions = signal<Permission[]>( null );
	pageData:DetailResolverData<Group>;
	get schema(){ return this.pageData.schema; }
	sideNav = model.required<DocItem>();
	tabIndex = signal<number>( LocalProfileStore.tabIndex('groupDetail') );
	ql:IGraphQL = inject( AccessService );
}