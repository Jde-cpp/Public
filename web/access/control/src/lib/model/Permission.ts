import { assert, Mutation, MutationType } from "jde-framework";
import { Resource } from "./Resource";
import { Role, RolePK } from "./Role";
import { Acl } from "./Acl";

export type PermissionPK=number;

export enum Rights{
	None=0,
	Create=0x1,
	Read=0x2,
	Update=0x4,
	Delete=0x8,
	Purge=0x10,
	Administer=0x20,
	Subscribe=0x40,
	Execute=0x80,
	All = 0xFF
};

export class Permission{
	constructor( obj:any ){
		this.id = obj.id;
		this.allowed = Permission.toRights( obj.allowed );
		this.denied = Permission.toRights( obj.denied );
		this.resource = new Resource( obj.resource );
	}
	static toRights( x:string|string[]|number|undefined ):Rights{
		let rights:Rights;
		if( typeof x == "number" ) rights = x as Rights;
		else if( typeof x == "string" ) rights = Rights[ x as keyof typeof Rights ];
		else if( Array.isArray(x) ) rights = x.reduce( (accum,right)=>accum | Rights[right as keyof typeof Rights], Rights.None );
		return rights;
	}

	static aclMutations( identityId:number, modified:Permission[], original:Permission[] ):Mutation[]{
		let y = [];
		for( let mod of modified ){
			let existing = original?.find( x=>x.id==mod.id );
			if( existing ){
				if( !mod.allowed && !mod.denied )
					y.push( new Mutation(Acl.typeName, null, { identity:{id:identityId}, permissionRight:{id:existing.id} }, MutationType.Purge) );
				else{
					let m = mod.mutation(existing);
					if( m )
						y.push( m );
				}
			}else{
				console.assert( !mod.id );
				y.push( new Mutation(Acl.typeName, null,
					{identity:{ id:identityId },permissionRight:{allowed:mod.allowed ?? 0, denied:mod.denied ?? 0, resource:{id:mod.resource.id}}},
					MutationType.Create/*,permissionRight{id}*/) );
				console.log( y[0].toString() );
			}
		}
		return y;
	}
	static roleMutations( rolePK:RolePK, alteredRows:Permission[], originalRows:Permission[]|null ):Mutation[]{
		let mutations = [];
		for( let altered of alteredRows ){
			let existing = originalRows.find( x=>x.id==altered.id );
			if( existing ){
				if( !altered.allowed && !altered.denied )
					mutations.push( new Mutation(Role.typeName, rolePK, {permissionRight:{id: existing.id}}, MutationType.Remove) );
				else{
					let m = altered.mutation(existing);
					if( m )
						mutations.push( m );
				}
			}else{
				assert( !altered.id );
				//addRole( id:42, allowed:255, denied:0, resource:{target:"users"} )
				mutations.push( new Mutation(Role.typeName, rolePK, {permissionRight:{allowed:altered.allowed ?? 0, denied:altered.denied ?? 0, resource:{target:altered.resource.target}}}, MutationType.Add) );
			}
		}
		return mutations;
	}
	mutation( original:Permission ):Mutation|null{
		let args = {};
		if( this.allowed!=original?.allowed )
			args["allowed"] = this.allowed;
		if( this.denied!=original?.denied )
			args["denied"] = this.denied;
		return Object.keys(args).length ? new Mutation( Permission.typeName, this.id, args, original==null ? MutationType.Create : MutationType.Update ) : null;
	}
	static arraysEqual( a:Permission[], b:Permission[] ):boolean{
		if (a == b) return true;
		if (a == null || b == null) return false;
		if (a.length !== b.length) return false;
		return a.every( (aValue,i)=>
			b.find( bValue=> aValue.id==bValue.id && aValue.allowed==bValue.allowed && aValue.denied==bValue.denied ) != null
		);
	}
	id: PermissionPK;
	allowed: Rights;
	denied: Rights;
	resource: Resource;
	static typeName = "PermissionRight";
};