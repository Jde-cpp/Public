import { Component, computed, model, Inject, OnDestroy, OnInit, signal, ViewChild, input, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { MatTabsModule } from '@angular/material/tabs';
import { MatToolbarModule } from '@angular/material/toolbar';
import { UaNode } from '../../../model/Node';
import { NodeRights } from './node-rights/node-rights';
//import { Gateway } from '../../../../services/gateway.service';
import { NodeRoute } from '../../../model/NodeRoute';
import { ProfileStore } from 'jde-spa';
import { AppService, Mutation, MutationType } from 'jde-framework';
import { ActivatedRoute } from '@angular/router';
import { Permission, Rights, Role } from 'jde-access';

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
			const criteria = this.node().nodeId.uaString();//e.g. "ns=4;i=6020" — the server's resource criteria for this node
			const vars = { schemaName: `opc.${this.accessResource()}`, target: "nodeIds", criteria };
			const q = 'roles{ id name permissionRight{ id allowed denied resource(schemaName:$schemaName, criteria:$criteria, target:$target){ id criteria } } }';
			const db = await this.appService.queryArray<any>( q, vars, (m)=>console.log(m) );
			const roles:RolePermission[] = db.map( (role:any)=>{
				const pr = role.permissionRight;//present only where a permission exists for this exact node; allowed/denied are [Right] name arrays
				return {
					roleId: role.id,
					roleName: role.name,
					permissionId: pr?.id ?? 0,
					allowed: Permission.toRights( pr?.allowed ?? [] ),
					denied: Permission.toRights( pr?.denied ?? [] ),
					resourceId: pr?.resource?.id ?? 0,
					criteria: pr?.resource?.criteria ?? criteria,
				};
			}).sort( (a,b)=>a.roleName.localeCompare(b.roleName) );
			this.original = roles;
			this.roles = roles.map( r=>({...r}) );
			this.isLoading.set( false );
		});
	}

	ngOnDestroy() {
		ProfileStore.setTabIndex( 'nodeAccess', this.tabIndex );
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

		let original = this.original.find( x=>x.roleId==role.roleId )!;
		let args:{allowed?: number, denied?: number} = {};
		if( role.allowed != original.allowed )
			args.allowed = role.allowed;
		if( role.denied != original.denied )
			args.denied = role.denied;
		let index = this.mutations().findIndex( x=>x.id==role.roleId );
		let mutation:Mutation|undefined = undefined;
		if( Object.keys(args).length>0 ){
			if( !args.allowed && !args.denied )
				mutation = new Mutation(Role.typeName, role.roleId, {permissionRight:{id: role.permissionId}}, MutationType.Remove );
			else{
				let resource:any = role.resourceId ? { id: role.resourceId } : { schemaName: `opc.${this.accessResource()}`, target: "nodeIds" };//was `schema:` — server ignored it and stored schemaName as "opc", putting the permission on the wrong resource
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

	access = Rights;//node ACLs use the generic Right enum (Create/Read/Update/Delete/Purge/Administer/Subscribe/Execute), same as the rest of access — NOT EAccess/EWriteAccess
	mutations = signal<Mutation[]>( [] );
	isLoading = signal( true );
	node = model.required<UaNode>();
	tabIndex:number = ProfileStore.tabIndex( 'nodeAccess' );
	original!:RolePermission[];

	accessResource = input.required<string>();
	route = model.required<NodeRoute>();
	roles: RolePermission[]=[];
	appService = inject(AppService);
}