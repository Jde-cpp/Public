import { cloneClassArray, Mutation, TargetRow } from "jde-framework";
import { Group } from "./Group";
import { Role } from "./Role";
import { Permission } from "./Permission";
import { Acl } from "./Acl";

export type UserPK = number;

export class User extends TargetRow<User>{
	constructor( obj:any ){
		super("User", obj);
		let roles = obj.roles ?? obj.childRoles;

		this.exponent = obj.exponent;
		this.groups = cloneClassArray( obj.groupings ?? obj.groups, Group );
		this.loginName = obj.loginName;
		this.modulus = obj.modulus;
		this.password = obj.password;
		this.permissions = cloneClassArray( obj.permissionRights ?? obj.permissions, Permission ) ?? [];
		this.provider = obj.provider;
		//"acl":[{"role":{"id":33,"name":"Opc Gateway Permissions","deleted":null},"identity":{"id":1}}]}
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
		else
			this.roles = cloneClassArray( obj.roles, Role ) ?? [];
	}
	override equals( row:User ):boolean{
		return super.equals(row)
			&& JSON.stringify(this.groups)==JSON.stringify(row.groups)
			&& JSON.stringify(this.permissions)==JSON.stringify(row.permissions)
			&& JSON.stringify(this.roles)==JSON.stringify(this.roles);
	}
	override mutation( original:User ):Mutation[]{
		console.assert( this.id==original.id, "User mutation id mismatch", this.id, original?.id );
		let propertyMutation = super.mutation( original );
		const permissionMutations = Permission.aclMutations( this.id, this.permissions, original?.permissions );
		const groupMutations = super.addRemoveMutations( Group.typeName, original.groups ?? [], this.groups, {memberId:this.id} );
		//createAcl( identity:{{ id:{} }}, role:{{id:{}}} )
		const roleMutations = Acl.roleMutations( this.id, original.roles ?? [], this.roles );
		return [...propertyMutation, ...permissionMutations, ...groupMutations, ...roleMutations];
	}
	exponent:number;
	groups: Group[];
	loginName: string;
	modulus:number;
	password: string;
	permissions: Permission[];
	get properties():User{ let properties = new User(this); properties.roles=undefined; properties.permissions=undefined; properties.groups=undefined; return properties; }
	provider: string;
	roles: Role[];
	override get collectionName():string{ return "users"; }
}