import { Permission } from "./Permission";
import { TargetRow } from "../../../../jde-framework/src/lib/model/ql/TargetRow";
import { cloneClassArray, Mutation, MutationType } from "jde-framework";
import { User, UserPK } from "./User";
import { Group, GroupPK } from "./Group";
import { Acl } from "./Acl";

export type RolePK = number;
export type RoleNK = string;
export class Role extends TargetRow<Role>{
	constructor( obj:any ){
		super( Role.typeName, obj );
		this.permissions = cloneClassArray( obj.permissionRights ?? obj.permissions, Permission ) ?? [];
		let childRoles = obj.roles ?? obj.childRoles;
		if( childRoles )
			this.childRoles = childRoles.map( (r)=>new Role(r) );
		if( obj.acl ){
			this.users = [];
			this.groups = [];
			for( let ac of obj.acl ){
				for( let identity of ac.identities )
				if( identity.isGroup ){
					this.groups.push( new Group(identity) );
				}else
					this.users.push( new User(identity) );
			}
		}
		else{
			this.users = cloneClassArray( obj.users, User );
			this.groups = cloneClassArray( obj.groups, Group );
		}
	}
	override equals( obj:Role ):boolean{
		const role = <Role><unknown>obj;
		return super.equals(obj) && JSON.stringify(this.permissions)==JSON.stringify(role.permissions);
	}
	override mutation( original:Role ):Mutation[]{
		let propertyies = super.mutation( original );
		const permissions = Permission.roleMutations( original.id, this.permissions, original.permissions );
		const childRoles = super.childMutations( this, original.childRoles ?? [], this.childRoles, {role:{}} );
		const groups = this.aclMutations<Group>( this.groups, original.groups ?? [] );
		const users = this.aclMutations<User>( this.users, original.users ?? [] );
		return [...propertyies, ...permissions, ...childRoles, ...groups, ...users];
	}

	private aclMutations<T extends TargetRow<T>>( modified:TargetRow<T>[], original:TargetRow<T>[] ):Mutation[]{
		let y = [];
		for( let mod of modified ){
			if( !original.find(x=>x.id==mod.id) )
				y.push( new Mutation(Acl.typeName, null, {identity:{ id:mod.id },role:{id: this.id}}, MutationType.Create) );
		}
		for( let existing of original ){
			if( !modified?.find(x=>x.id==existing.id) )
				y.push( new Mutation(Acl.typeName, null, {identity:{ id:existing.id },role:{id: this.id}}, MutationType.Purge) );
		}
		return y;
	}

	childRoles: Role[];
	groups: Group[];
	permissions: Permission[];
	get properties():Role{ let properties = new Role(this); properties.childRoles=undefined; properties.permissions=undefined; return properties; }
	override get collectionName():string{ return "roles"; }
	users: User[];
	static typeName = "Role";
}