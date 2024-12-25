#include <jde/access/Authorize.h>
#include <jde/db/usings.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/User.h>
#include "../../../../Framework/source/collections/Collections.h"

#define let const auto
namespace Jde::Access{

	α Authorize::Test( str schemaName, str resourceName, ERights rights, UserPK executer, SL sl )ε->void{
		optional<ResourcePK> resourcePK;
		Jde::sl _{Mutex};
		if( auto schemaResorces = SchemaResources.find(schemaName); schemaResorces!=SchemaResources.end() )
			resourcePK = Find( schemaResorces->second, resourceName );
		if( !resourcePK )//not enabled
			return;

		if( auto user = Users.find(executer); user!=Users.end() ){
			THROW_IFSL( user->second.IsDeleted, "[{}]User is deleted.", executer.Value );
			let configured = user->second.ResourceRights( *resourcePK );
			THROW_IFSL( !empty(configured.Denied & rights), "[{}]User denied '{}' access to '{}'.", executer.Value, ToString(rights), resourceName );
			THROW_IFSL( empty(configured.Allowed & rights), "[{}]User does not have '{}' access to '{}'.", executer.Value, ToString(rights), resourceName );
		}
		else
			throw Exception{ sl, ELogLevel::Debug, "[{}]User not found.", executer.Value };
	}
	α Authorize::TestAdmin( ResourcePK resourcePK, UserPK executer, SL sl )ε->void{
		Jde::sl _{Mutex};
		auto resource=Resources.find( resourcePK );
		if( resource!=Resources.end() && !resource->second.IsDeleted )
			TestAdmin( resource->second, executer, sl );
	}
	α Authorize::TestAdmin( str resourceTarget, UserPK executer, SL sl )ε->void{
		Jde::sl l{ Mutex };
		auto resource = find_if( Resources, [&](let& r){ return r.second.Target==resourceTarget; } );
		if( resource!=Resources.end() && !resource->second.IsDeleted )
			TestAdmin( resource->second, executer, sl );
	}
	α Authorize::TestAdmin( const Resource& resource, UserPK executer, SL sl )ε->void{
		auto user = Users.find(executer);
		if( user==Users.end() )
			THROW_IFSL( user==Users.end(), "[{}]User not found.", executer.Value );
		THROW_IFSL( user->second.IsDeleted, "[{}]User is deleted.", executer .Value);
		let configured = user->second.ResourceRights( resource.PK );
		THROW_IFSL( !empty(configured.Denied & ERights::Administer), "[{}]User denied admin access to '{}'.", executer.Value, resource.Target );
		THROW_IFSL( empty(configured.Allowed & ERights::Administer), "[{}]User does not have admin access to '{}'.", executer.Value, resource.Target );
	}

	α Authorize::RecursiveUsers( GroupPK groupPK, const ul& l, bool clear )ι->flat_set<UserPK>{
		flat_set<UserPK> users;
		auto group = Groups.find( groupPK );
		if( group==Groups.end() || group->second.IsDeleted )
			return users;

		for( auto member : group->second.Members ){
			if( member.IsUser() ){
				users.emplace( member.UserPK() );
				if( auto user = clear ? Users.end() : Users.find(member.UserPK()); user!=Users.end() )
					user->second.Clear();
			}
			else{
				let groupUsers = RecursiveUsers( member.GroupPK(), l );
				users.insert( groupUsers.begin(), groupUsers.end() );
			}
		}
		return users;
	}

	α Authorize::AddToGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void{
		ul l{ Mutex };
		flat_set<UserPK> users;
		auto& existing = Groups.try_emplace( groupPK, Group{groupPK, false} ).first->second;
		for( let& member : members ){
			if( auto pkUser = Users.find(UserPK{member}); pkUser!=Users.end() ){
				existing.Members.emplace( pkUser->first );
				pkUser->second.Clear();
				users.emplace( pkUser->first );
			}
			else{
				GroupPK childGroup{ member };
				existing.Members.emplace( childGroup );
				let groupUsers = RecursiveUsers( childGroup, l, true );
				for( let user : groupUsers )
					users.emplace( user );
			}
		}
		if( users.size() )
			SetUserPermissions( move(users), l );
	}
	α Authorize::RestoreGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		if( auto p = Groups.find(groupPK); p!=Groups.end() ){
			p->second.IsDeleted = false;
			RecalcGroupMembers( groupPK, l );
		}
	}

	α Authorize::RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK::Type> members )ι->void{
		ul l{ Mutex };
		flat_set<UserPK> users;
		auto group = Groups.find( groupPK );
		if( group==Groups.end() )
			return;
		for( let& member : members ){
			auto existing = group->second.Members.find( UserPK{member} );//compare on UserPK.
			if( existing==group->second.Members.end() )
				continue;
			if( auto pkUser = existing->IsUser() ? Users.find(existing->UserPK()) : Users.end(); pkUser!=Users.end() ){
				pkUser->second.Clear();
				users.emplace( pkUser->first );
			}
			else if( !existing->IsUser() ){
				for( let user : RecursiveUsers(existing->GroupPK(), l, true) )
					users.emplace( user );
			}
			group->second.Members.erase( existing );
		}
		if( users.size() )
			SetUserPermissions( move(users), l );
	}
	α Authorize::RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove )ι->void{
		auto users = RecursiveUsers( groupPK, l, true );
		if( remove )
		Groups.erase( groupPK );
		if( users.size() )
			SetUserPermissions( move(users), l );
	}
	α Authorize::AddAcl( IdentityPK::Type userGroupPK, uint permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void{
		ul l{ Mutex };
		const PermissionRole permissionRole{ std::in_place_index<0>, permissionPK };
		const Permission permission{ permissionPK, resourcePK, allowed, denied };
		Permissions.emplace( permissionPK, permission );
		auto user = Users.find({userGroupPK});
		let identityPK = user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
		Acl.emplace( identityPK, permissionRole );
		if( user!=Users.end() )
			user->second += permission;
		else
			RecalcGroupMembers( identityPK.GroupPK(), l );
	}

	α Authorize::AddAcl( IdentityPK::Type userGroupPK, RolePK rolePK )ι->void{
		ul l{ Mutex };
		auto user = Users.find({userGroupPK});
		let identityPK = user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
		Acl.emplace( identityPK, PermissionRole{std::in_place_index<1>, rolePK} );
		if( user!=Users.end() )
			AddPermission( identityPK, PermissionRole{std::in_place_index<1>, rolePK}, {user->first}, l );
		else
			RecalcGroupMembers( identityPK.GroupPK(), l );
	}

	α Authorize::CreateResource( Resource&& resource )ε->void{
		ul _{Mutex};
		Resources[resource.PK] = move(resource);
	}
	α Authorize::UpdateResourceDeleted( str schemaName, const jobject& args, bool restored )ε->void{
		ul _{Mutex};
		let pk = Json::FindNumber<ResourcePK>( args, "id" );
		let target = Json::FindSV( args, "target" );
		auto pkResource = find_if( Resources, [&](auto&& pkResource){
			let& r = pkResource.second;
			return (pk && *pk==r.PK) || ( r.Schema==schemaName && target && *target==r.Target );
		} );
		THROW_IF( pkResource==Resources.end(), "Resource not found schema:{}, args:{}", schemaName, serialize(args) );
		auto& resource = pkResource->second;

		resource.IsDeleted = restored ? optional<DB::DBTimePoint>{} : DB::DBClock::now();
		if( resource.Filter.empty() ){
			if( auto resources = resource.IsDeleted ? SchemaResources.find( resource.Schema ) : SchemaResources.end(); resources!=SchemaResources.end() )
				resources->second.erase( resource.Target );
			else if( !resource.IsDeleted )
				SchemaResources.try_emplace( schemaName ).first->second.emplace( resource.Target, resource.PK );
		}
	}


	α Authorize::CreateUser( UserPK userPK )ι->void{
		ul _{Mutex};
		Users.emplace( userPK, User{userPK, false} );
	}
	α Authorize::DeleteUser( UserPK identityPK )ι->void{
		ul _{Mutex};
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.IsDeleted = true;
	}
	α Authorize::RestoreUser( UserPK identityPK )ι->void{
		ul _{Mutex};
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.IsDeleted = false;
	}
	α Authorize::DeleteGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		RecalcGroupMembers( groupPK, l, true );
	}
	//TODO test on deleted groups.
	α Authorize::TestAddGroupMember( GroupPK parentGroupPK/*groupD*/, flat_set<IdentityPK::Type>&& memberPKs, SL sl )ε->void{
		std::shared_lock _{ Mutex };
		for( let memberPK : memberPKs ){
			if( Users.contains({memberPK}) )
				continue;
			GroupPK childGroup{ memberPK };/*GroupA*/
			THROW_IFSL( childGroup==parentGroupPK, "Group cannot be a member of itself." );
			if( IsChild(Groups, childGroup, parentGroupPK) )
				throw Exception{ sl, ELogLevel::Debug, "Group '{}' cannot be a member of '{}' because it is a ancester.", childGroup.Value, parentGroupPK.Value };
		}
	}
	α Authorize::PurgeGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		auto p = Groups.find( groupPK );
		if( p==Groups.end() )
			return;
		let deleted = p->second.IsDeleted;
		Groups.erase( p );
		if( !deleted )
			RecalcGroupMembers( groupPK, l );
	}

	α Authorize::TestAddRoleMember( RolePK parent, RolePK child, SL sl )ε->void{
		THROW_IFSL( parent==child, "Role cannot be a member of itself." );
		function<bool(RolePK,RolePK)> isChild = [&](RolePK parent, RolePK child)->bool {
			auto children = Roles.find( parent );
			if( children==Roles.end() )
				return false;
			for( PermissionRole member : children->second.Members ){
				if( member.index()==1 && (get<1>(member)==child || isChild( get<1>(member), child )) )
					return true;
			}
			return false;
		};
		std::shared_lock _{ Mutex };
		THROW_IFSL( isChild(child, parent), "Role '{}' cannot be a member of '{}' because it is a ancester.", child, parent );
	}
	α Authorize::AddRolePermission( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, str resourceName )ι->void{
		ul l{ Mutex };
		if( auto permssion = Permissions.find(member); permssion!=Permissions.end() ){
			permssion->second.Allowed = allowed;
			permssion->second.Denied = denied;
		}
		else if( auto resource = find_if(Resources, [&](let& r){ return r.second.Target==resourceName;}); resource!=Resources.end() )
			Permissions.emplace( member, Permission{member, resource->first, allowed, denied} );

		auto role = Roles.try_emplace(rolePK, rolePK, false);
		role.first->second.Members.emplace( PermissionRole{std::in_place_index<0>, member} );
		Recalc( l );
	}
	α Authorize::AddRoleChild( RolePK parentRolePK, RolePK childRolePK )ι->void{
		ul l{ Mutex };

		auto role = Roles.try_emplace(parentRolePK, parentRolePK, false);
		role.first->second.Members.emplace( PermissionRole{std::in_place_index<1>,childRolePK} );
		Recalc( l );
	}

	α Authorize::RemoveFromRole(	RolePK rolePK, flat_set<PermissionIdentityPK> toRemove )ι->void{
		if( !toRemove.size() )
			return;
		ul l{ Mutex };
		for( let& member : toRemove ){
			auto role = Roles.find( rolePK );
			ASSERT( role!=Roles.end() );
			if( role==Roles.end() )
				continue;
			auto& members = role->second.Members;
			for( auto p = members.begin(); p!=members.end(); ++p ){
				if( member==std::visit([](auto id)->PermissionIdentityPK{return id;}, *p) ){
					members.erase( p );
					break;
				}
			}
		}
		Recalc( l );
	}

	α	Authorize::DeleteRestoreRole( RolePK rolePK, bool deleted )ι->void{
		ul l{ Mutex };
		if( auto p = Roles.find(rolePK); p==Roles.end() )
			p->second.IsDeleted = deleted;
		Recalc( l );//not sure a better way than recalc all users.
	}
	α Authorize::PurgeRole( RolePK rolePK )ι->void{
		ul l{ Mutex };
		auto p = Roles.find( rolePK );
		if( p==Roles.end() )
			return;
		let deleted = p->second.IsDeleted;
		Roles.erase( p );
		if( !deleted )
			Recalc( l );
	}

	α Authorize::AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void{
		if( auto pkUser = identityPK.IsUser() ? Users.find(identityPK.UserPK()) : Users.end(); pkUser!=Users.end() ){
			if( !users.empty() && !users.contains(pkUser->first) )
				return;
			if( auto p = permissionRole.index()==0 ? Permissions.find(get<0>(permissionRole)) : Permissions.end(); p!=Permissions.end() )
				pkUser->second += p->second;
			else if( auto rolePermissions = permissionRole.index()==1 ? Roles.find(get<1>(permissionRole)) : Roles.end(); rolePermissions!=Roles.end() && !rolePermissions->second.IsDeleted ){
				for( let& rolePermission : rolePermissions->second.Members )
					AddPermission( identityPK, rolePermission, users, l );
			}
		}
		else if( auto group = identityPK.IsUser() ? Groups.end() : Groups.find(identityPK.GroupPK()); group!=Groups.end() ){
			for( auto member : group->second.Members )
				AddPermission( member, permissionRole, users, l );//user
		}
	}
	α Authorize::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		ul l{Mutex};
		auto p = Permissions.find(permissionPK); THROW_IF( p==Permissions.end(), "[{}]Permission not found", permissionPK );
		p->second.Update( allowed, denied );
		for( auto& user : Users )
			user.second.UpdatePermission( permissionPK, allowed, denied );
	}
	α Authorize::Recalc( const ul& l )ι->void{
		for( auto& user : Users )
			user.second.Clear();
		SetUserPermissions( {}, l );
	}
	α Authorize::SetUserPermissions( flat_set<UserPK>&& users, const ul& l )ι->void{
		for( let& [identityPK,permissionRole] : Acl )
			AddPermission( identityPK, permissionRole, users, l );
	}
}