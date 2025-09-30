#include <jde/access/Authorize.h>
#include <jde/fwk/str.h>
#include <jde/db/usings.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/User.h>
#include <jde/fwk/utils/collections.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr ELogTags _ptags{ ELogTags::Access | ELogTags::Pedantic };

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
		else if( executer.Value!=UserPK::System )
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
		if( executer==UserPK{UserPK::System} )
			return;
		auto user = Users.find( executer );
		if( user==Users.end() )
			THROW_IFSL( user==Users.end(), "[{}]User not found.", executer.Value );
		THROW_IFSL( user->second.IsDeleted, "[{}]User is deleted.", executer .Value);
		let configured = user->second.ResourceRights( resource.PK );
		THROW_IFSL( !empty(configured.Denied & ERights::Administer), "[{}]User denied admin access to '{}'.", executer.Value, resource.Target );
		THROW_IFSL( empty(configured.Allowed & ERights::Administer), "[{}]User does not have admin access to '{}'.", executer.Value, resource.Target );
	}
	α Authorize::TestAdminPermission( PermissionPK permissionPK, UserPK userPK, SL sl )ε->void{
		Jde::sl l{Mutex};
		if( auto permission = Permissions.find(permissionPK); permission!=Permissions.end() ){
			l.unlock();
			TestAdmin( permission->second.ResourcePK, userPK, sl );
		}
		else
			THROW( "[{}]Permission not found.", permissionPK );
	}

	α Authorize::Rights( str schemaName, str resourceName, UserPK executer )ι->ERights{
		optional<ResourcePK> resourcePK;
		Jde::sl _{Mutex};
		if( auto schemaResources = SchemaResources.find(schemaName); schemaResources!=SchemaResources.end() )
			resourcePK = Find( schemaResources->second, resourceName );
		if( !resourcePK )//not enabled
			return ERights::All;

		auto user = Users.find(executer);
		if( user==Users.end() || user->second.IsDeleted )
			return ERights::None;

		auto rights = user->second.ResourceRights( *resourcePK );
		return rights.Allowed & ~rights.Denied;
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
				ASSERT( groupPK!=member.GroupPK() );
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
				TRACET( _ptags, "[{}+{}]AddToGroup", groupPK.Value, childGroup.Value );
				let groupUsers = RecursiveUsers( childGroup, l, true );
				for( let user : groupUsers )
					users.emplace( user );
			}
		}
		if( users.size() )
			SetUserPermissions( move(users), l );
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


	α Authorize::RestoreGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		if( auto p = Groups.find(groupPK); p!=Groups.end() ){
			p->second.IsDeleted = false;
			RecalcGroupMembers( groupPK, l );
		}
	}

	α Authorize::RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove )ι->void{
		auto users = RecursiveUsers( groupPK, l, true );
		if( remove )
			Groups.erase( groupPK );
		if( users.size() )
			SetUserPermissions( move(users), l );
	}
	α Authorize::AddAcl( IdentityPK::Type userGroupPK, PermissionPK permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void{
		ul l{ Mutex };
		const PermissionRole permissionRole{ std::in_place_index<0>, permissionPK };
		ASSERT( resourcePK );
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
	α Authorize::RemoveAcl( IdentityPK::Type userGroupPK, PermissionRole rolePK )ι->void{
		ul l{ Mutex };
		let identityPK = ToIdentityPK( userGroupPK, l );
		auto permissionRoles = Acl.equal_range( ToIdentityPK(userGroupPK, l) );
		for( auto p=permissionRoles.first; p!=permissionRoles.second; ++p ){
			if( p->second==rolePK ){
				Acl.erase( p );
				break;
			}
		}
		if( identityPK.IsUser() )
			SetUserPermissions( {identityPK.UserPK()}, l );
		else
			RecalcGroupMembers( identityPK.GroupPK(), l );
	}

	α Authorize::ToIdentityPK( IdentityPK::Type userGroupPK, const ul& )Ι->IdentityPK{
		auto user = Users.find({userGroupPK});
		return user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
	}

	α Authorize::CreateResource( Resource&& resource )ε->void{
		ul _{Mutex};
		Resources[resource.PK] = move(resource);
	}
	α Authorize::UpdateResourceDeleted( ResourcePK pk, sv schemaName, const jobject& args, bool restored )ε->void{
		ul _{Mutex};
		if( !pk )
			pk = Json::FindNumber<ResourcePK>( args, "id" ).value_or(0);
		let target = Json::FindSV( args, "target" );
		auto pkResource = find_if( Resources, [&](auto&& pkResource){
			let& r = pkResource.second;
			return (pk && pk==r.PK) || ( r.Schema==schemaName && target && *target==r.Target );
		} );
		THROW_IFX( pkResource==Resources.end(), Exception(SRCE_CUR, ELogLevel::Debug, "Resource not found pk: {}, schema:'{}', args:'{}'", pk, schemaName, serialize(args)) );
		auto& resource = pkResource->second;

		resource.IsDeleted = restored ? optional<DB::DBTimePoint>{} : DB::DBClock::now();
		if( resource.Filter.empty() ){
			if( auto resources = resource.IsDeleted ? SchemaResources.find( resource.Schema ) : SchemaResources.end(); resources!=SchemaResources.end() ){
				resources->second.erase( resource.Target );
				DBGT( _ptags, "[{}.{}.{}]Deleted from schema resource.", resource.Schema, resource.Target, resource.PK );
			}
			else if( !resource.IsDeleted ){
				SchemaResources.try_emplace( string{resource.Schema} ).first->second.emplace( resource.Target, resource.PK );
				DBGT( _ptags, "[{}.{}.{}]Restored from schema resource.", resource.Schema, resource.Target, resource.PK );
			}
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
	α Authorize::PurgeUser( UserPK identityPK )ι->void{
		ul _{Mutex};
		Users.erase( identityPK );
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
	//TODO test on deleted members.
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
	α Authorize::AddRolePermission( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, sv resourceName )ι->void{
		ul l{ Mutex };
		if( auto permssion = Permissions.find(member); permssion!=Permissions.end() ){
			permssion->second.Allowed = allowed;
			permssion->second.Denied = denied;
		}
		else if( auto resource = find_if(Resources, [&](let& r){ return r.second.Target==resourceName;}); resource!=Resources.end() )
			Permissions.emplace( member, Permission{member, resource->first, allowed, denied} );

		auto role = Roles.try_emplace( rolePK, rolePK, false );
		role.first->second.Members.emplace( PermissionRole{std::in_place_index<0>, member} );
		Recalc( l );
		TRACET( _ptags, "[{}+{}]Added role permission.", rolePK, member );
	}
	α Authorize::AddRoleChild( RolePK parentRolePK, vector<RolePK>&& childRolePKs )ι->void{
		ul l{ Mutex };
		auto role = Roles.try_emplace( parentRolePK, parentRolePK, false );
		for( let childRolePK : childRolePKs )
			role.first->second.Members.emplace( PermissionRole{std::in_place_index<1>,childRolePK} );

		Recalc( l );
		TRACET( _ptags, "[{}+{}]Added role child.", parentRolePK, Str::Join(childRolePKs) );
	}

	α Authorize::RemoveRoleChildren(	RolePK rolePK, flat_set<PermissionPK> toRemove )ι->void{
		if( !toRemove.size() )
			return;
		ul l{ Mutex };
		auto role = Roles.find( rolePK );
		ASSERT( role!=Roles.end() );
		if( role==Roles.end() )
			return;
		for( let& member : toRemove ){
			auto& members = role->second.Members;
			for( auto p = members.begin(); p!=members.end(); ++p ){
				if( member==std::visit([](auto id)->PermissionRightsPK{return id;}, *p) ){
					members.erase( p );
					break;
				}
			}
		}
		Recalc( l );
	}

	α	Authorize::DeleteRestoreRole( RolePK rolePK, bool deleted )ι->void{
		ul l{ Mutex };
		if( auto p = Roles.find(rolePK); p!=Roles.end() )
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