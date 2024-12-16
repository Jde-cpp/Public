#pragma once
#include <jde/access/IAcl.h>
#include "accessInternal.h"
#include "types/Resource.h"
#include "types/Group.h"
#include "types/Role.h"
#include "types/User.h"

namespace Jde::Access{
	struct Permission;// struct User; struct Group;

	struct Authorize : IAcl{
		Authorize()ε{}
		α Test( str schemaName, str resourceName, ERights rights, UserPK userPK, SRCE )ε->void override;
		α TestAdmin( str resource, UserPK userPK, SL sl )ε->void;
		α TestAdmin( ResourcePK resourcePK, UserPK userPK, SL sl )ε->void;
		α CalculateUsers( flat_set<UserPK>&& users, const ul& l )ι->void;
		α RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove=false )ι->void;
		α Recalc( const ul& l )ι->void;
		α AddAcl( IdentityPK::Type identityPK, uint permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void;
		α AddAcl( IdentityPK::Type userGroupPK, RolePK rolePK )ι->void;

		α CreateUser( UserPK userPK )ι->void;
		α DeleteUser( UserPK identityPK )ι->void;
		α RestoreUser( UserPK identityPK )ι->void;
		α DeleteGroup( GroupPK identityPK )ι->void;
		α DeleteRestoreRole( RolePK rolePK, bool deleted )ι->void;
		α AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void;
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α RecursiveUsers( GroupPK groupPK, const ul& l, bool clear=false )ι->flat_set<UserPK>;

		α AddToGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void;
		α RestoreGroup( GroupPK groupPK )ι->void;
		α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void;
		α TestAddGroupMember( GroupPK groupPK, flat_set<IdentityPK::Type>&& memberPKs, SRCE )ε->void;

		α TestAddRoleMember( RolePK parent, RolePK child, SRCE )ε->void;

		mutable std::shared_mutex Mutex;
		flat_map<string, flat_map<string,Access::ResourcePK>> SchemaResources;//schemaName, resourceJsonName, resourcePK -- active only
		flat_map<ResourcePK,Resource> Resources; //includes inactive resources.
		flat_map<UserPK,User> Users;

		flat_map<PermissionPK,Permission> Permissions;
		flat_map<GroupPK,Group> Groups;
		flat_map<RolePK,Role> Roles;
		flat_multimap<IdentityPK,PermissionRole> Acl;
	private:
		α TestAdmin( const Resource& resource, UserPK userPK, SL sl )ε->void;
	};
}