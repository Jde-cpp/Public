import { cloneClassArray, Mutation, TargetRow } from "jde-framework";
import { Role } from "./Role";
import { Permission } from "./Permission";
import { User } from "./User";
import { Acl } from "./Acl";

export type GroupPK = number;

export class Group extends TargetRow<Group>{
	constructor( obj:any ){
		super(Group.typeName, obj);
		this.roles = obj.roles ? [] : undefined;
		if( obj.acl ){
			this.roles = [];
			this.permissions = [];
			for( let member of obj.acl ){
				if( member.role ){
					this.roles.push( new Role(member.role) );
				}else if( member.permissionRight ){
					this.permissions.push( new Permission(member.permissionRight) );
				}
			}
		}
		else{
			this.roles = cloneClassArray( obj.roles, Role );
			this.permissions = cloneClassArray( obj.permissions, Permission );
		}
		if( obj.members ){
			this.childGroups = [];
			this.users = [];
			for( let identity of obj.members ?? [] ){
				if( identity.isGroup )
					this.childGroups.push( new Group(identity) );
				else
					this.users.push( new User(identity) );
			}
		}
		else{
			this.childGroups = cloneClassArray( obj.childGroups, Group );
			this.users = cloneClassArray( obj.users, User );
		}
	}
	override equals( group:Group ):boolean{
		return super.equals(group) && JSON.stringify(this.roles)==JSON.stringify(group.roles);
	}
	override mutation( original:Group ):Mutation[]{
		const propertiesMutation = super.mutation( original );
		const roleMutations = Acl.roleMutations( this.id, original.roles ?? [], this.roles );
		const permissionMutations = Permission.aclMutations( this.id, this.permissions ?? [], original.permissions  ?? [] );

		let groupMutations = super.childMutations( this, original.childGroups ?? [], this.childGroups );
		groupMutations.forEach( m=>m.args = {memberId:m.args.id} );

		let users = super.childMutations( this, original.users ?? [], this.users );
		users.forEach( m=>m.args = {memberId:m.args.id} );
		return [...propertiesMutation, ...groupMutations, ...roleMutations, ...permissionMutations, ...users];
	}
	roles: Role[];
	childGroups: Group[];
	permissions: Permission[];
	users: User[];

	get properties():Group{
		let properties = new Group(this);
		properties.roles=undefined;
		properties.childGroups=undefined;
		return properties;
	}
	static typeName = "Grouping";
}