import { Component, computed, effect, signal, inject } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { CommonModule } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { arraysEqual, cloneClassArray, DetailPage, IGraphQL, Properties, QLSelector, Style, TableSettings, TargetRow, toIdArray} from 'jde-framework';

import { RolePK } from '../../model/role';
import { PermissionTable } from '../../shared/permissions/permission-table';
import { Permission } from '../../model/permission';
import { AccessService } from '../../services/access-service';
import { GroupPK } from '../../model/group';
import { User } from '../../model/user';
import { KeyProperties } from './key-properties/key-properties';

@Component( {
    templateUrl: './user-detail.html',
		styleUrls: ['./user-detail.scss'],
		//the trailing class is load-bearing:  Angular hashes a component's *shape* into its style-encapsulation id and leaves
		//the class name out, so four routed pages that now share DetailPage and this host string could collide with NG0912.
		host: {class:'main-content mat-drawer-container my-content user-detail'},
    imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, KeyProperties, PermissionTable, QLSelector]
})
export class UserDetail extends DetailPage<User>{
	constructor(){
		super( 'userDetail' );
		effect(() => {
			if( this.groups() && !arraysEqual(TargetRow.idArray(this.row.groups ?? []),this.groups().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.roles() && !arraysEqual(TargetRow.idArray(this.row.roles), this.roles().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.permissions() && !Permission.arraysEqual(this.row?.permissions ?? [], this.permissions()) )//value-compare like role-detail — identity arraysEqual on the clones marked the page dirty on load
				this.isChanged.set( true );
		});
	}

	protected override get ctor(){ return User; }
	protected override onRow(){
		this.groups.set( new SelectionModel<GroupPK>(true, TargetRow.idArray(this.row.groups)) );
		this.permissions.set( cloneClassArray(this.row.permissions ?? [], Permission) );
		this.roles.set( new SelectionModel<RolePK>(true, TargetRow.idArray(this.row.roles ?? [])) );
	}
	protected override upsert():User{
		return new User( { ...this.properties(), permissions: this.permissions(), roles: this.roles().selected, groups: toIdArray(this.groups().selected) } );
	}

	public copy( existing:User ):User{
		return new User( existing );
	}

	get user(){ return this.row; }//the template's name for it
	groups = signal<SelectionModel<GroupPK>>( null as any );
	permissions = signal<Permission[]>( null as any );
	roles = signal<SelectionModel<RolePK>>( null as any );

	userTableSettings = userTableSettings;
	providerName = computed<string>( ()=>{
		const value = this.properties()?.provider as string|number|undefined;
		return typeof value=="number"
			? this.schema.enums.get( "Provider" )?.find( (o)=>o.id==value )?.name ?? ""
			: value ?? "";
	});
	isKeyProvider = computed<boolean>( ()=>this.providerName().toLowerCase()=="key" );
	excludedColumns = [...userTableSettings.excludedColumns!, ...keyFields];
	override ql:IGraphQL = inject( AccessService );
}

const keyFields = ["modulus", "exponent", "issuer", "subjectAlt", "distinguished", "expiration"];

export const userTableSettings:TableSettings = {
	excludedColumns: ["isGroup"],
	columns: [
		{ name:"name", style: new Style(300) },
		{ name:"provider", style: new Style(100) },
		"description"
	]
 }
 export const resourceTableSettings:TableSettings = {
	canAdd: false,
	canPurge: false,
	canNavigate: false,//there is no 'resources/:target' route - resources are server-defined rows with no detail page
	columns: [
		{ name:"name", style: new Style(300) },
		"description"
	]
 }
