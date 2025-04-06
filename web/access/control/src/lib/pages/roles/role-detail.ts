import { Component, computed, effect, OnInit, OnDestroy, Inject, signal, ViewEncapsulation, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import {ActivatedRoute, NavigationEnd, Params, Router, RouterModule, Routes} from '@angular/router';
import { MatDialog } from '@angular/material/dialog';

import { ComponentPageTitle } from 'jde-material';

import { Properties, IErrorService, IGraphQL, IProfile, Settings, TableSchema, arraysEqual, cloneClassArray, TargetRow, toIdArray} from 'jde-framework';
import { HttpErrorResponse } from '@angular/common/http';
import { MatTabsModule } from '@angular/material/tabs';
import { Role, RolePK } from '../../model/Role';
import { IRoleData, UserSettings } from '../../services/role.resolver';
import { PermissionTable } from '../../shared/permissions/permission-table';
import { MatIcon } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { Permission } from '../../model/Permission';
import { QLSelector } from '../../../../../jde-framework/src/lib/pages/ql/selector/ql-selector';
import { AccessService, DetailResolverData } from 'jde-access';
import { SelectionModel } from '@angular/cdk/collections';
import { GroupPK } from '../../model/Group';
import { UserPK } from '../../model/User';

@Component( {
    selector: 'roles',
    templateUrl: './role-detail.html',
		styleUrls: ['./role-detail.scss'],
		host: {class:'main-content mat-drawer-container my-content'},
    imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, PermissionTable, QLSelector]
})
export class RoleDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IProfile') private profileService: IProfile, @Inject('IErrorService') private snackbar: IErrorService ){
		effect(() => {
			if( !this.properties() )
				return;
			if( !this.properties().canSave )
				this.isChanged.set( false );
			else if( !this.properties().equals(this.role.properties) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.childRoles() && !arraysEqual(TargetRow.idArray(this.role.childRoles), this.childRoles().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.groups() && !arraysEqual(TargetRow.idArray(this.role.groups),this.groups().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.users() && !arraysEqual(TargetRow.idArray(this.role.users),this.users().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.permissions() && !Permission.arraysEqual(this.role.permissions, this.permissions()) )
				this.isChanged.set( true );
		});
	}
	ngOnDestroy(){
		this.profile.save();
	}
	async ngOnInit(){
		this.pageData = await this.route.data["value"]["pageData"];
		this.role = new Role( this.pageData.row );
		this.pageData.row = null;

		this.properties.set( this.role.properties );
		this.permissions.set( cloneClassArray(this.role.permissions, Permission) );
		this.childRoles.set( new SelectionModel<RolePK>(true, TargetRow.idArray(this.role.childRoles)) );
		this.groups.set( new SelectionModel<GroupPK>(true, TargetRow.idArray(this.role.groups)) );
		this.users.set( new SelectionModel<UserPK>(true, TargetRow.idArray(this.role.users)) );

		this.isLoading.set( false );
	}
	tabIndexChanged( index:number ){ this.profile.value.tabIndex = index;}

	async onSubmitClick(){
		try{
			const upsert = new Role( {
				id:this.properties().id,
				...this.properties(),
				permissions: this.permissions(),
				roles: this.childRoles().selected,
				groups: toIdArray(this.groups().selected),
				users: toIdArray(this.users().selected)
			});
			const mutation = upsert.mutation( this.role );
			await this.ql.mutation( mutation );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.error( "Save failed.", e );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}
	public copy( existing:Role ):Role{
		return new Role( existing );
	}
	role:Role;
	pageData:DetailResolverData<Role>;
	ctor:new (item: any) => any = Role;
	isChanged = signal<boolean>( false );
	isLoading = signal<boolean>( true );
	get profile(){ return this.pageData.pageSettings.profile;}

	permissions = signal<Permission[]>( null );
	properties = signal<Role>( null );
	childRoles = signal<SelectionModel<RolePK>>( null );

	groups = signal<SelectionModel<RolePK>>( null );

	users = signal<SelectionModel<RolePK>>( null );
	get schema(){ return this.pageData.schema; }

	ql:IGraphQL = inject( AccessService );
}
