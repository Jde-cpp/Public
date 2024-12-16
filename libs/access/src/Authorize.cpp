#include "Authorize.h"
#include "types/Group.h"
#include "types/User.h"
#include "../../../../Framework/source/collections/Collections.h"

#define let const auto
namespace Jde::Access{

	α Authorize::Test( str schemaName, str resourceName, ERights rights, UserPK userPK, SL sl )ε->void{
		optional<ResourcePK> resourcePK;
		Jde::sl _{Mutex};
		if( auto schemaResorces = SchemaResources.find(schemaName); schemaResorces!=SchemaResources.end() )
			resourcePK = Find( schemaResorces->second, resourceName );
		if( !resourcePK )//not enabled
			return;

		if( auto user = Users.find(userPK); user!=Users.end() ){
			THROW_IFSL( user->second.Deleted, "[{}]User is deleted.", userPK.Value );
			let configured = user->second.ResourceRights( *resourcePK );
			THROW_IFSL( !empty(configured.Denied & rights), "[{}]User denied '{}' access to '{}'.", userPK.Value, ToString(rights), resourceName );
			THROW_IFSL( empty(configured.Allowed & rights), "[{}]User does not have '{}' access to '{}'.", userPK.Value, ToString(rights), resourceName );
		}
		else
			throw Exception{ sl, ELogLevel::Debug, "[{}]User not found.", userPK.Value };
	}
	α Authorize::TestAdmin( ResourcePK resourcePK, UserPK userPK, SL sl )ε->void{
		Jde::sl _{Mutex};
		auto resource=Resources.find( resourcePK );
		if( resource!=Resources.end() && !resource->second.Deleted )
			TestAdmin( resource->second, userPK, sl );
	}
	α Authorize::TestAdmin( str resourceTarget, UserPK userPK, SL sl )ε->void{
		Jde::sl l{ Mutex };
		auto resource = find_if( Resources, [&](let& r){ return r.second.Target==resourceTarget; } );
		if( resource!=Resources.end() && !resource->second.Deleted )
			TestAdmin( resource->second, userPK, sl );
	}
	α Authorize::TestAdmin( const Resource& resource, UserPK userPK, SL sl )ε->void{
		auto user = Users.find(userPK);
		if( user==Users.end() )
			THROW_IFSL( user==Users.end(), "[{}]User not found.", userPK.Value );
		THROW_IFSL( user->second.Deleted, "[{}]User is deleted.", userPK .Value);
		let configured = user->second.ResourceRights( resource.PK );
		THROW_IFSL( !empty(configured.Denied & ERights::Administer), "[{}]User denied admin access to '{}'.", userPK.Value, resource.Target );
		THROW_IFSL( empty(configured.Allowed & ERights::Administer), "[{}]User does not have admin access to '{}'.", userPK.Value, resource.Target );
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
			CalculateUsers( move(users), l );
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
			CalculateUsers( move(users), l );
	}
	α Authorize::RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove )ι->void{
		auto users = RecursiveUsers( groupPK, l, true );
		if( remove )
		Groups.erase( groupPK );
		if( users.size() )
			CalculateUsers( move(users), l );
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

	α Authorize::CreateUser( UserPK userPK )ι->void{
		ul _{Mutex};
		Users.emplace( userPK, User{userPK, false} );
	}
	α Authorize::DeleteUser( UserPK identityPK )ι->void{
		ul _{Mutex};
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.Deleted = true;
	}
	α Authorize::RestoreUser( UserPK identityPK )ι->void{
		ul _{Mutex};
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.Deleted = false;
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
	α Authorize::TestAddRoleMember( RolePK parent, RolePK child, SL sl )ε->void{
		THROW_IFSL( parent==child, "Role cannot be a member of itself." );
		function<bool(RolePK,RolePK)> isChild = [&](RolePK parent, RolePK child)->bool {
			auto children = Roles.find( parent );
			if( children==Roles.end() )
				return false;
			for( let permissionRole : children->second.Members ){
				if( permissionRole.index()==0 )
					continue;
				if( get<1>(permissionRole)==child || isChild( get<1>(permissionRole), child ) )
					return true;
			}
			return false;
		};
		std::shared_lock _{ Mutex };
		THROW_IFSL( isChild(child, parent), "Role '{}' cannot be a member of '{}' because it is a ancester.", child, parent );
	}

	α	Authorize::DeleteRestoreRole( RolePK rolePK, bool deleted )ι->void{
		ul l{ Mutex };
		auto p = Roles.find( rolePK );
		ASSERT( p!=Roles.end() );
		if( p==Roles.end() )
			return;
		p->second.Deleted = deleted;
		Recalc( l );//not sure a better way than recalc all users.
	}

	α Authorize::AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void{
		if( auto pkUser = identityPK.IsUser() ? Users.find(identityPK.UserPK()) : Users.end(); pkUser!=Users.end() ){
			if( !users.empty() && !users.contains(pkUser->first) )
				return;
			if( auto p = permissionRole.index()==0 ? Permissions.find(get<0>(permissionRole)) : Permissions.end(); p!=Permissions.end() )
				pkUser->second += p->second;
			else if( auto rolePermissions = Roles.find(get<1>(permissionRole)); rolePermissions!=Roles.end() && !rolePermissions->second.Deleted ){
				for( let& rolePermission : rolePermissions->second.Members )
					AddPermission( identityPK, rolePermission, users, l );
			}
		}
		else if( auto group = identityPK.IsUser() ? Groups.end() : Groups.find(identityPK.GroupPK()); group!=Groups.end() ){
			for( auto member : group->second.Members )
				AddPermission( member, permissionRole, users, l );//user
		}
	}
	α Authorize::CalculateUsers( flat_set<UserPK>&& users, const ul& l )ι->void{
		for( let& [identityPK,permissionRole] : Acl )
			AddPermission( identityPK, permissionRole, users, l );
	}
}