import { Component, effect, OnInit, OnDestroy, signal, inject, Inject, model } from '@angular/core';
import { SelectionModel } from '@angular/cdk/collections';
import { CommonModule } from '@angular/common';
import { ActivatedRoute, Router } from '@angular/router';
import { MatButtonModule } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MatTabsModule } from '@angular/material/tabs';

import { ComponentPageTitle, DocItem } from 'jde-spa';
import { arraysEqual, cloneClassArray, DetailResolverData, IErrorService, IGraphQL, LocalProfileStore, Properties, QLSelector, TargetRow, toIdArray} from 'jde-framework';

import { RolePK } from '../../model/Role';
import { PermissionTable } from '../../shared/permissions/permission-table';
import { Permission } from '../../model/Permission';
import { AccessService } from '../../services/access.service';
import { GroupPK } from '../../model/Group';
import { User } from '../../model/User';

@Component( {
    templateUrl: './user-detail.html',
		styleUrls: ['./user-detail.scss'],
		host: {class:'main-content mat-drawer-container my-content'},
    imports: [CommonModule, MatButtonModule, MatIcon, MatTabsModule, Properties, PermissionTable, QLSelector]
})
export class UserDetail implements OnDestroy, OnInit{
	constructor( private route: ActivatedRoute, private router:Router, private componentPageTitle:ComponentPageTitle, @Inject('IErrorService') private snackbar: IErrorService ){
		effect(() => {
			if( !this.properties() )
				return;
			if( !this.properties().canSave )
				this.isChanged.set( false );
			else if(  !this.properties().equals(this.user.properties) )
				this.isChanged.set( true );
		});

		effect(() => {
			if( this.groups() && !arraysEqual(TargetRow.idArray(this.user.groups ?? []),this.groups().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( this.roles() && !arraysEqual(TargetRow.idArray(this.user.roles), this.roles().selected) )
				this.isChanged.set( true );
		});
		effect(() => {
			if( !arraysEqual(this?.user?.permissions, this.permissions()) )
				this.isChanged.set( true );
		});
		route.data.subscribe( (data)=>{
			this.pageData = data["pageData"];
			this.user = new User( this.pageData.row );
			this.sideNav.set( this.pageData.routing );
			this.pageData.row = null;
			this.properties.set( this.user.properties );
			this.groups.set( new SelectionModel<GroupPK>(true, TargetRow.idArray(this.user.groups)) );
			this.permissions.set( cloneClassArray(this.user.permissions ?? [], Permission) );
			this.roles.set( new SelectionModel<RolePK>(true, TargetRow.idArray(this.user.roles ?? [])) );
			this.componentPageTitle.title = this.user.name;
			this.isLoading.set( false );
		});
	}
	ngOnDestroy(){
		LocalProfileStore.setTabIndex( 'userDetail', this.tabIndex() );
	}
	ngOnInit(){
		this.sideNav.set( this.pageData.routing );
	}
	onTabIndexChanged( index:number ){ this.tabIndex.set(index);}

	async onSubmitClick(){
		try{
			const upsert = new User( { ...this.properties(), permissions: this.permissions(), roles: this.roles().selected, groups: toIdArray(this.groups().selected) } );
			const mutation = upsert.mutation( this.user );
			await this.ql.mutate( mutation );
			this.router.navigate( ['..'], { relativeTo: this.route } );
		}catch(e){
			this.snackbar.exceptionInfo( e, "Save failed.", (m)=>console.log(m) );
		}
	}
	public onCancelClick(){
		this.router.navigate( ['..'], { relativeTo: this.route } );
	}
	public copy( existing:User ):User{
		return new User( existing );
	}

	user:User;
	ctor:new (item: any) => any = User;
	isChanged = signal<boolean>( false );
	isLoading = signal<boolean>( true );
	properties = signal<User>( null );
	groups = signal<SelectionModel<GroupPK>>( null );
	permissions = signal<Permission[]>( null );
	roles = signal<SelectionModel<RolePK>>( null );

	sideNav = model.required<DocItem>();

	pageData:DetailResolverData<User>;
	get schema(){ return this.pageData.schema; }
	tabIndex = signal<number>( LocalProfileStore.tabIndex('userDetail') );
	ql:IGraphQL = inject( AccessService );
}