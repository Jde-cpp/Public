#pragma once
#include <jde/access/IAcl.h>
#include "types/Resource.h"
#include "types/Group.h"
#include "types/Role.h"
#include "types/User.h"

struct UA_Server;
namespace Jde::Access{
	namespace Server{ struct AuthenticateAwait; struct LoginAwait; }
	struct Listener; struct Loader; struct Permission;

	struct Authorize /*final*/ : IAcl, std::enable_shared_from_this<Authorize>{
		Authorize( string app )ι:_app{move(app)}{}
		virtual ~Authorize()=default;

		α Test( str schemaName, str resourceName, ERights rights, UserPK userPK, SRCE )ε->void override;
		α Rights( str schemaName, str resourceName, UserPK executer )ι->ERights override;
		α UserName( UserPK userPK )ι->string override;

		α AddResource( ResourcePK resourcePK, string schema, string resourceTarget, string criteria )ι->void;
		//By value:  the protected overload's pointer aliases Resources, a flat_map any concurrent insert reallocates, so nothing may
		//carry it past the lock (access-review3 #19).  A shared lock, as this reads only.
		α FindResource( const Resource& resource )Ι->optional<Resource>{ Jde::sl l{Mutex}; auto p = FindResource( resource, l ); return p ? optional<Resource>{*p} : optional<Resource>{}; }
		α FindActiveResourcePK( string schema, str resourceName, str criteria )ι->optional<ResourcePK>{ Jde::sl _{Mutex}; return FindActiveResourcePK(schema, resourceName, criteria, _); }
		α GetSchema( str resourceTarget, SL sl )ε->string;

		α TestAdmin( str resource, UserPK userPK, SRCE )ε->void;
		//The gate on a role/acl grant for (schema, target, criteria).  Remote - the schema's registered IAdminAcl, the OpcServer,
		//which alone knows which resource governs a node - when one is registered and its registrant still passes TestSchemaAdmin;
		//else TestAdminLocal, pre-completed (appserver-review3 #13).
		α TestAdmin( str schema, str resource, str criteria, UserPK userPK, SRCE )ι->up<AnyVoidAwait>;
		//The flat rule, on this cache alone:  the active (schema,target,criteria) row when there is one - a mapped criteria is its
		//own resource, root does not inherit down - else the target's criteria-less root row, which an unmapped criteria inherits
		//as an unmapped node inherits it in OpcAuthorize::UserRights.  Neither active = not enabled, passes as Test does.
		α TestAdminLocal( str schema, str resource, str criteria, UserPK userPK, SRCE )ε->void;
		α TestAdmin( ResourcePK resourcePK, UserPK userPK, SRCE )ε->void;
		α TestAdminPermission( PermissionPK permissionPK, UserPK userPK, SRCE )ε->void;
		//May userPK stand in for the schema and answer its admin checks (AddAdminAuthorizer)?  Administer on every active
		//criteria-less resource of the schema.  A schema with no active root passes:  unknown, or every root deleted, is a
		//no-op rather than a denial, so an instance may register before its resources exist (appserver-review3 #4, rejected -
		//the denial was tried and removed, do not reinstate it without changing that call).
		α TestSchemaAdmin( str schema, UserPK userPK, SRCE )ε->void;
		struct AdminAuthorizer{ sp<IAdminAcl> Acl; UserPK User; };//User: the registrant, re-tested with TestSchemaAdmin at each check so a registrant whose rights went falls back to the local rule.
		α AddAdminAuthorizer( str schemaName, sp<IAdminAcl> authorizer, UserPK registrant )ι->void;
		α RemoveAdminAuthorizer( const sp<IAdminAcl>& authorizer )ι->void;//deregister on disconnect - registrations are otherwise permanent and go stale.

		α TestAddGroupMember( GroupPK groupPK, flat_set<IdentityPK::Type>&& memberPKs, SRCE )ε->void;
		α TestAddRoleMember( RolePK parent, RolePK child, SRCE )ε->void;
	protected:
		Ŧ FindResource( const Resource& resource, T& l )Ι->const Resource*;
		Ŧ FindActiveResourcePK( str schemaName, str resourceName, str criteria, T& l )Ι->optional<ResourcePK>;

		string _app;
		mutable std::shared_mutex Mutex;
		/// Active only <schemaName, <resourceJsonName,<criteria, resourcePK>>>
		flat_map<string, flat_map<string,flat_map<string,Access::ResourcePK>>> SchemaResources;
		flat_map<UserPK,User> Users;
		α SetUserPermissions( flat_set<UserPK>&& users, const ul& l )ι->void;
		α RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove=false )ι->void;
		α Recalc( const ul& l )ι->void;
		α RecursiveUsers( GroupPK groupPK, const ul& l, bool clear=false )ι->flat_set<UserPK>;
		α RecursiveUsers( GroupPK groupPK, const ul& l, bool clear, flat_set<GroupPK>& visited )ι->flat_set<UserPK>;
		α FindAdminAuthorizer( str schemaName )ι->optional<AdminAuthorizer>;

		α AddAclEntry( IdentityPK identityPK, PermissionRole permissionRole, const ul& l )ι->void;
		α PurgeIdentity( IdentityPK identityPK, const ul& l )ι->void;
		α AddAcl( IdentityPK::Type userGroupPK, PermissionPK permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void;
		α AddAcl( IdentityPK::Type userGroupPK, RolePK rolePK )ι->void;
		α RemoveAcl( IdentityPK::Type userGroupPK, PermissionRole rolePK )ι->void;

		α AddToGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void;
		α DeleteGroup( GroupPK identityPK )ι->void;
		α RestoreGroup( GroupPK groupPK )ι->void;
		α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void;
		α PurgeGroup( GroupPK groupPK )ι->void;

		α AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void;
		α AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, flat_set<GroupPK>& visitedGroups, const ul& l )ι->void;
		α AddUserPermissions( User& user, PermissionRole permissionRole, flat_set<RolePK>& visitedRoles )ι->void;
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;

		β CreateResource( Resource&& resource )ε->void;
		β UpdateResourceDeleted( ResourcePK pk, sv schemaName, const jobject& args, bool restored )ε->void;

		α DeleteRestoreRole( RolePK rolePK, bool deleted )ι->void;
		α PurgeRole( RolePK rolePK )ι->void;
		α AddRolePermission( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, const jobject& resource )ι->void;
		α AddRoleChild( RolePK parentRolePK, vector<RolePK>&& childRolePK )ι->void;
		α RemoveRoleChildren(	RolePK rolePK, flat_set<PermissionRightsPK> toRemove )ι->void;

		α CreateUser( UserPK userPK )ι->void;
		α DeleteUser( UserPK identityPK )ι->void;
		α RestoreUser( UserPK identityPK )ι->void;
		α PurgeUser( UserPK identityPK )ι->void;

		α TestAdmin( const Resource& resource, UserPK userPK, SL sl )ε->void;
		α ToIdentityPK( IdentityPK::Type userGroupPK, const ul& l )Ι->IdentityPK;

		/// Includes inactive resources.
		flat_map<ResourcePK,Resource> Resources;

		flat_map<PermissionPK,Permission> Permissions;
		flat_map<GroupPK,Group> Groups;
		flat_map<RolePK,Role> Roles;
		flat_multimap<IdentityPK,PermissionRole> Acl;
	private:
		concurrent_flat_map<string,AdminAuthorizer> _adminAuthorizers;
		friend struct AccessListener; friend struct Loader; friend struct ConfigureAwait; friend struct Server::AuthenticateAwait; friend struct Server::LoginAwait;
	};

	Ŧ Authorize::FindResource( const Resource& resource, T& l )Ι->const Resource*{
		auto pk = resource.PK;
		if( !pk && resource.Schema.size() && resource.Target.size() )
			pk = FindActiveResourcePK( resource.Schema, resource.Target, resource.Criteria, l ).value_or( 0 );
		if( auto p = pk ? Resources.find(pk) : Resources.end(); p!=Resources.end() )
			return &p->second;
		//Only a criteria-less request may fall back to the criteria-less row.  Without that guard a *criteria-scoped* lookup
		//that missed - its row not cached yet, which is the ordinary case for a resource created by the same mutation that
		//grants on it - resolved to the target's root row instead, and AddRolePermission then wrote the node's rights over
		//the root's.  A node-scoped grant silently rewriting the root grant is the opposite of what it says (opcserver-review3
		//L30, found fixing that test).  Missing now returns null, and AddRolePermission's `new resource` branch caches the
		//row from the payload, which carries its pk.
		if( resource.Target.size() && resource.Criteria.empty() ){
			for( const auto& [existingPK,existing] : Resources ){
				if( (resource.Schema.empty() || existing.Schema==resource.Schema) && existing.Target==resource.Target && existing.Criteria.empty() )
					return &existing;
			}
		}
		return nullptr;
	}
	Ŧ Authorize::FindActiveResourcePK( str schemaName, str resourceTarget, str criteria, T& /*lock*/ )Ι->optional<ResourcePK>{
		if( auto schemaResources = SchemaResources.find(schemaName); schemaResources!=SchemaResources.end() ){
			if( auto targetResources = schemaResources->second.find(resourceTarget); targetResources!=schemaResources->second.end() ){
				auto& criteras = targetResources->second;
				if( criteras.contains({}) )// if permisions are enabled
					return Find(criteras, criteria);
			}
		}
		return {};
	}
}