import { Component, computed, model, Inject, OnDestroy, OnInit, signal, ViewChild, input, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { MatTabsModule } from '@angular/material/tabs';
import { MatToolbarModule } from '@angular/material/toolbar';
import { UaNode } from '../../../model/Node';
import { NodeRights } from './node-rights/node-rights';
//import { Gateway } from '../../../../services/gateway.service';
import { EAccess, EWriteAccess } from '../../../model/types';
import { NodeRoute } from '../../../model/NodeRoute';
import { AppService, LocalProfileStore, Mutation, MutationType } from 'jde-framework';
import { ActivatedRoute } from '@angular/router';
import { Permission, Role } from 'jde-access';

export type RolePermission = {
	roleId: number;
	roleName: string;
	permissionId: number;
	allowed:number;
	denied:number;
	resourceId: number;
	criteria: string;
};
@Component( {
	selector: 'node-access',
	templateUrl: './node-access.html',
	styleUrls: ['./node-access.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatTabsModule, MatToolbarModule, MatButtonModule, NodeRights]
})
export class NodeAccess implements OnInit, OnDestroy{
	constructor( private actRoute: ActivatedRoute )
	{}

	async ngOnInit(): Promise<void> {
		this.actRoute.data.subscribe( async (data)=>{
			let criteria: string[] = [];
			let prev = null;
			for( let segment of this.route().browsePath.split('/') ){
				let path = prev ? prev + '/' + segment : segment;
				criteria.push( segment=='' ? null : segment );
				prev = segment;
			}
			const vars = { schemaName: `opc.${this.accessResource()}`, target: "node", criteria: criteria };
			let q = 'roles{ id name permissionRight{id allowed denied resource(schemaName:$schemaName, criteria:$criteria, target:$node){ id criteria }';
			let db = await this.appService.queryArray<any>( q, vars, (m)=>console.log(m) );
			let applicable:Record<string,RolePermission> = {};
			for( const role of db ){
				const rights = role.permissionRight ?? { id: 0, allowed: 0, denied: 0, resource: { id: 0, criteria: '' } };
				let criteria = rights.resource.criteria;
				if( !applicable[role.name] || applicable[role.name].criteria.length > criteria.length )
					applicable[role.name] = { roleId: role.id, roleName: role.name, permissionId: rights.id, allowed: Permission.toRights( rights.allowed ), denied: Permission.toRights( rights.denied ), resourceId: rights.resource.id, criteria: rights.resource.criteria ?? prev };
			}
			let roles:RolePermission[] = [];
			for( const roleName in applicable )
				roles.push( applicable[roleName] );
			roles = roles.sort( (a,b)=>a.roleName.localeCompare(b.roleName) );
			this.original = roles;
			for( const role of roles )
				this.roles.push( {...role} );
			this.isLoading.set( false );
		});
	}

	ngOnDestroy() {
		LocalProfileStore.setTabIndex( 'nodeAccess', this.tabIndex );
  }
	async save(){
		await this.appService.mutate( this.mutations(), (m)=>console.log(m) );
	}
	onTabIndexChanged( index:number ){ this.tabIndex = index; }
	onToggle( event:{ role: RolePermission, rights:number } ):void{
		let role = event.role;
		let rights = event.rights;
				if( NodeRights.isAllowed(role, rights) ){//allowed->denied
			role.allowed &= ~rights;
			role.denied |= rights;
		}else if( NodeRights.isDenied(role, rights) ){//denied->none
			role.denied &= ~rights;
		}else{//none->allowed
			if( rights==0 )
				role.allowed = 0;
			else
				role.allowed |= rights;
		}

		let original = this.original.find( x=>x.roleId==role.roleId );
		let args:{allowed?: number, denied?: number} = {};
		if( role.allowed != original.allowed )
			args.allowed = role.allowed;
		if( role.denied != original.denied )
			args.denied = role.denied;
		let index = this.mutations().findIndex( x=>x.id==role.roleId );
		let mutation:Mutation = null;
		if( Object.keys(args).length>0 ){
			if( !args.allowed && !args.denied )
				mutation = new Mutation(Role.typeName, role.roleId, {permissionRight:{id: role.permissionId}}, MutationType.Remove );
			else{
				let resource = role.resourceId ? { id: role.resourceId } : { schema: `opc.${this.accessResource()}`, target: "nodeIds" };
				resource["criteria"] = role.criteria ? role.criteria : null;
				mutation = new Mutation( Role.typeName, role.roleId, { permissionRight: {allowed: role.allowed, denied: role.denied, resource: resource} }, MutationType.Add );
			}
		}

		let mutations = [];
		this.mutations().forEach( (m,i)=>{
			if( i!=index )
				mutations.push( m );
			else if( mutation )
				mutations.push( mutation );
		} );
		if( index==-1 && mutation )
			mutations.push( mutation );
		this.mutations.set( mutations );
		if( mutation )
			console.log( mutation.toString() );
	}

	access = EAccess;
	writeAccess = EWriteAccess;
	mutations = signal<Mutation[]>( [] );
	isLoading = signal( true );
	node = model.required<UaNode>();
	tabIndex:number = LocalProfileStore.tabIndex( 'nodeAccess' );
	original:RolePermission[];

	accessResource = input.required<string>();
	route = model.required<NodeRoute>();
	rights = EAccess;
	roles: RolePermission[]=[];
	writeRoles: RolePermission[];
	appService = inject(AppService);
}