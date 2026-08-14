#include <jde/access/Authorize.h>
#include <jde/fwk/str.h>
#include <jde/db/usings.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/User.h>
#include <jde/access/AccessException.h>
#include <jde/fwk/utils/collections.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr ELogTags _ptags{ ELogTags::Access | ELogTags::Pedantic };

	α Authorize::AddAdminAuthorizer( str schemaName, sp<IAdminAcl> authorizer )ι->void{
		_adminAuthorizers.insert_or_assign( schemaName, move(authorizer) );//not emplace: a restarted app must replace its stale registration, not be silently discarded behind the closed one.
	}
	α Authorize::RemoveAdminAuthorizer( const sp<IAdminAcl>& authorizer )ι->void{
		_adminAuthorizers.erase_if( [&](let& pair){ return pair.second==authorizer; } );//drop every schema this session authorized, so FindAdminAuthorizer falls back to the local check instead of a dead stream.
	}
	α Authorize::FindAdminAuthorizer( str schemaName )ι->sp<IAdminAcl>{
		sp<IAdminAcl> authorizer;
		_adminAuthorizers.cvisit( schemaName, [&](let& pair){authorizer = pair.second;} );
		return authorizer ? authorizer : shared_from_this();
	}
	α Authorize::AddResource( ResourcePK resourcePK, string schema, string resourceTarget, string criteria )ι->void{
		Jde::sl _{ Mutex };
		SchemaResources[schema][resourceTarget][criteria] = resourcePK;
	}

	α Authorize::FindResource( const Resource& resource, ul& l )Ι->const Resource*{
		auto pk = resource.PK;
		if( !pk && resource.Schema.size() && resource.Target.size() )
			pk = FindActiveResourcePK( resource.Schema, resource.Target, resource.Criteria, l ).value_or( 0 );
		if( auto p = pk ? Resources.find(pk) : Resources.end(); p!=Resources.end() )
			return &p->second;
		else if( /*resource.Schema.empty() &&*/ resource.Target.size() ){
			for( let& [pk,existing] : Resources ){
				if( (resource.Schema.empty() || existing.Schema==resource.Schema) && existing.Target==resource.Target && existing.Criteria.empty() )
					return &existing;
			}
		}
		return nullptr;
	}
	α Authorize::GetSchema( str resourceTarget, SL sl )ε->string{
		Jde::sl _{ Mutex };
		for( let& [_,resource] : Resources ){
			if( resource.Target==resourceTarget && !resource.Schema.contains('.') ) //exclude opc schemas which can have same target
				return resource.Schema;
		}
		THROWSL( "Schema not found for resource target '{}'.", resourceTarget );
	}
	α Authorize::Test( str schemaName, str resourceName, ERights rights, UserPK executer, SL sl )ε->void{
		Jde::sl l{ Mutex };
		auto resourcePK = FindActiveResourcePK( schemaName, resourceName, {}, l );
		if( !resourcePK )//not enabled
			return;

		if( auto user = Users.find(executer); user!=Users.end() ){
			THROW_IFX( user->second.IsDeleted, Exception(sl, ELogLevel::Debug, "[{}]User is deleted.", executer.Value) );
			let configured = user->second.ResourceRights( *resourcePK );
			THROW_IFX( !empty(configured.Denied & rights), Exception(sl, ELogLevel::Debug, "[{}]User denied '{}' access to '{}'.", executer.Value, ToString(rights), resourceName) );
			THROW_IFX( empty(configured.Allowed & rights), Exception(sl, ELogLevel::Debug, "[{}]User does not have '{}' access to '{}'.", executer.Value, ToString(rights), resourceName) );
		}
		else if( executer.Value!=UserPK::System )
			throw Exception{ sl, ELogLevel::Debug, "[{}]User not found.", executer.Value };
	}
	α Authorize::TestAdmin( ResourcePK resourcePK, UserPK executer, SL sl )ε->void{
		Jde::sl _{ Mutex };
		auto resource=Resources.find( resourcePK );
		if( resource!=Resources.end() && !resource->second.IsDeleted )
			TestAdmin( resource->second, executer, sl );
	}

	α Authorize::TestAdmin( str resourceTarget, UserPK executer, SL sl )ε->void{
		Jde::sl l{ Mutex };
		auto resource = find_if( Resources, [&](let& r){return r.second.Target==resourceTarget && !r.second.Schema.contains('.');} );//exclude opc schemas which can have same target
		if( resource!=Resources.end() && !resource->second.IsDeleted )
			TestAdmin( resource->second, executer, sl );
	}
	α Authorize::TestAdmin( str schema, str resource, str criteria, UserPK userPK, SL sl )ι->up<AnyVoidAwait>{
		auto authorizer = FindAdminAuthorizer( schema );
		return authorizer->TestAdmin( resource, criteria, userPK, sl );
	}
	α Authorize::TestAdmin( str resource, str /*criteria*/, UserPK userPK, SL sl )ι->up<AnyVoidAwait>{
		up<Exception> error;
		try{
			TestAdmin( resource, userPK, sl ); //TODO handle criteria
		}
		catch( Exception& e ){ error = e.Move(); }
		catch( runtime_error& e ){ error = mu<Exception>( move(e) ); }
		return mu<AnyCompletedAwait>( move(error), sl );
	}
	α Authorize::TestAdmin( const Resource& resource, UserPK executer, SL sl )ε->void{
		if( executer==UserPK{UserPK::System} )
			return;
		auto user = Users.find( executer );
		THROW_IFX( user==Users.end(), Access::AccessException(sl, executer, "User not found.") );
		THROW_IFX( user->second.IsDeleted, Access::AccessException(sl, executer, "User is deleted.") );
		let configured = user->second.ResourceRights( resource.PK );
		THROW_IFX( !empty(configured.Denied & ERights::Administer), Access::AccessException(sl, executer, "User denied admin access to '{}'.", resource.Target) );
		THROW_IFX( empty(configured.Allowed & ERights::Administer), Access::AccessException(sl, executer, "User does not have admin access to '{}'.", resource.Target) );
	}
	α Authorize::TestAdminPermission( PermissionPK permissionPK, UserPK userPK, SL sl )ε->void{
		Jde::sl l{ Mutex };
		auto permission = Permissions.find( permissionPK );
		THROW_IF( permission==Permissions.end(), "[{}]Permission not found.", permissionPK );
		let resourcePK = permission->second.ResourcePK;
		l.unlock();
		TestAdmin( resourcePK, userPK, sl );
	}

	α Authorize::Rights( str schemaName, str resourceName, UserPK executer )ι->ERights{
		Jde::sl _{ Mutex };
		auto resourcePK = FindActiveResourcePK( schemaName, resourceName, {}, _ );
		if( !resourcePK )//not enabled
			return ERights::All;

		auto user = Users.find( executer );
		if( user==Users.end() )
			return executer.Value==UserPK::System ? ERights::All : ERights::None;//System early-passes in Test/TestAdmin - stay consistent.
		if( user->second.IsDeleted )
			return ERights::None;

		auto rights = user->second.ResourceRights( *resourcePK );
		return rights.Allowed & ~rights.Denied;
	}
	α Authorize::UserName( UserPK userPK )ι->string{
		Jde::sl _{ Mutex };
		if( auto user = Users.find(userPK); user!=Users.end() )
			return user->second.Name;
		else
			return std::to_string( userPK );
	}

	α Authorize::RecursiveUsers( GroupPK groupPK, const ul& l, bool clear )ι->flat_set<UserPK>{
		flat_set<GroupPK> visited;
		return RecursiveUsers( groupPK, l, clear, visited );
	}
	α Authorize::RecursiveUsers( GroupPK groupPK, const ul& l, bool clear, flat_set<GroupPK>& visited )ι->flat_set<UserPK>{
		flat_set<UserPK> users;
		auto group = visited.emplace( groupPK ).second ? Groups.find( groupPK ) : Groups.end();//visited guards cycles in existing data.
		if( group==Groups.end() || group->second.IsDeleted )
			return users;

		for( auto member : group->second.Members ){
			if( member.IsUser() ){
				users.emplace( member.UserPK() );
				if( auto user = clear ? Users.find(member.UserPK()) : Users.end(); user!=Users.end() )
					user->second.Clear();
			}
			else{
				let groupUsers = RecursiveUsers( member.GroupPK(), l, clear, visited );
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
	α Authorize::AddAclEntry( IdentityPK identityPK, PermissionRole permissionRole, const ul& )ι->void{
		let range = Acl.equal_range( identityPK );
		for( auto p=range.first; p!=range.second; ++p ){
			if( p->second==permissionRole )
				return;//multimap - a duplicate would survive RemoveAcl, which erases only the first match.
		}
		Acl.emplace( identityPK, permissionRole );
	}
	α Authorize::AddAcl( IdentityPK::Type userGroupPK, PermissionPK permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void{
		ul l{ Mutex };
		const PermissionRole permissionRole{ std::in_place_index<0>, permissionPK };
		ASSERT( Resources.find(resourcePK)!=Resources.end() );
		//access_ac_upsert_permission is an upsert on (identity, resource) - a re-grant returns the same pk carrying new rights.
		auto existing = Permissions.find( permissionPK );
		let changed = existing!=Permissions.end() && (existing->second.Allowed!=allowed || existing->second.Denied!=denied || existing->second.ResourcePK!=resourcePK);
		Permissions.insert_or_assign( permissionPK, Permission{permissionPK, resourcePK, allowed, denied} );
		auto user = Users.find( {userGroupPK} );
		let identityPK = user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
		AddAclEntry( identityPK, permissionRole, l );
		if( changed )
			Recalc( l );//rebuild everything - the pk may be cached on identities other than this one.
		else if( user!=Users.end() ){
			user->second.Clear();//rebuild, operator+= can only raise rights.
			SetUserPermissions( {user->first}, l );
		}
		else
			RecalcGroupMembers( identityPK.GroupPK(), l );
	}

	α Authorize::AddAcl( IdentityPK::Type userGroupPK, RolePK rolePK )ι->void{
		ul l{ Mutex };
		auto user = Users.find( {userGroupPK} );
		let identityPK = user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
		AddAclEntry( identityPK, PermissionRole{std::in_place_index<1>, rolePK}, l );
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
		if( identityPK.IsUser() ){
			if( auto user = Users.find(identityPK.UserPK()); user!=Users.end() )
				user->second.Clear();
			SetUserPermissions( {identityPK.UserPK()}, l );
		}
		else
			RecalcGroupMembers( identityPK.GroupPK(), l );
	}

	α Authorize::ToIdentityPK( IdentityPK::Type userGroupPK, const ul& )Ι->IdentityPK{
		auto user = Users.find( {userGroupPK} );
		return user!=Users.end() ? IdentityPK{ user->first } : IdentityPK{ GroupPK{userGroupPK} };
	}

	α Authorize::CreateResource( Resource&& resource )ε->void{
		ul _{ Mutex };
		Resources[resource.PK] = move( resource );
	}
	α Authorize::UpdateResourceDeleted( ResourcePK pk, sv schemaName, const jobject& args, bool restored )ε->void{
		ul _{ Mutex };
		if( !pk )
			pk = Json::FindNumber<ResourcePK>( args, "id" ).value_or( 0 );
		let target = Json::FindSV( args, "target" );
		auto pkResource = find_if( Resources, [&](auto&& pkResource){
			let& r = pkResource.second;
			return ( pk && pk==r.PK ) || ( r.Schema==schemaName && target && *target==r.Target );
		} );
		// ie Testing schema where testing app isn't started.
		THROW_IFX( pkResource==Resources.end(), Exception(SRCE_CUR, ELogLevel::Debug, "Resource not found pk: {}, schema:'{}', args:'{}'", pk, schemaName, serialize(args)) );
		auto& resource = pkResource->second;

		resource.IsDeleted = restored ? optional<DB::DBTimePoint>{} : DB::DBClock::now();
		if( resource.Criteria.empty() ){
			if( auto resources = resource.IsDeleted ? SchemaResources.find(resource.Schema) : SchemaResources.end(); resources!=SchemaResources.end() ){
				resources->second.erase( resource.Target );
				DBGT( _ptags, "[{}.{}.{}]Deleted from schema resource.", resource.Schema, resource.Target, resource.PK );
			}
			else if( !resource.IsDeleted ){
				auto& targetResources = SchemaResources.try_emplace( string{resource.Schema} ).first->second;
				auto& criteras = targetResources.try_emplace( resource.Target ).first->second;
				criteras.try_emplace( {}, resource.PK );
				DBGT( _ptags, "[{}.{}.{}]Restored from schema resource.", resource.Schema, resource.Target, resource.PK );
			}
		}
	}


	α Authorize::CreateUser( UserPK userPK )ι->void{
		ul _{ Mutex };
		Users.emplace( userPK, User{userPK, "", false} );
	}
	α Authorize::DeleteUser( UserPK identityPK )ι->void{
		ul _{ Mutex };
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.IsDeleted = true;
	}
	//a purged identity's acl rows and memberships are inert (lookups skip what isn't in Users/Groups) but would leak for the process lifetime.
	α Authorize::PurgeIdentity( IdentityPK identityPK, const ul& )ι->void{
		Acl.erase( identityPK );
		for( auto group=Groups.begin(); group!=Groups.end(); ++group )
			group->second.Members.erase( identityPK );//IdentityPK orders on Underlying(), so this matches a user or group member.
	}
	α Authorize::PurgeUser( UserPK identityPK )ι->void{
		ul l{ Mutex };
		Users.erase( identityPK );
		PurgeIdentity( identityPK, l );
	}
	α Authorize::RestoreUser( UserPK identityPK )ι->void{
		ul _{ Mutex };
		if( auto p = Users.find(identityPK); p!=Users.end() )
			p->second.IsDeleted = false;
	}
	α Authorize::DeleteGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		auto p = Groups.find( groupPK );
		if( p==Groups.end() || p->second.IsDeleted )
			return;
		auto users = RecursiveUsers( groupPK, l, true );//collect+clear while still active, RecursiveUsers early-outs on a deleted group.
		p->second.IsDeleted = true;//soft delete, symmetric with DeleteUser - RestoreGroup needs the row.
		if( users.size() )
			SetUserPermissions( move(users), l );
	}
	//TODO test on deleted members.
	α Authorize::TestAddGroupMember( GroupPK parentGroupPK/*groupD*/, flat_set<IdentityPK::Type>&& memberPKs, SL sl )ε->void{
		std::shared_lock _{ Mutex };
		for( let memberPK : memberPKs ){
			if( Users.contains({memberPK}) )
				continue;
			GroupPK childGroup{ memberPK };/*GroupA*/
			THROW_IFX( childGroup==parentGroupPK, Exception(sl, ELogLevel::Debug, "Group cannot be a member of itself.") );
			if( IsChild(Groups, childGroup, parentGroupPK) )
				throw Exception{ sl, ELogLevel::Debug, "Group '{}' cannot be a member of '{}' because it is a ancester.", childGroup.Value, parentGroupPK.Value };
		}
	}
	α Authorize::PurgeGroup( GroupPK groupPK )ι->void{
		ul l{ Mutex };
		auto p = Groups.find( groupPK );
		if( p==Groups.end() )
			return;
		if( p->second.IsDeleted )
			Groups.erase( p );//members were cleared+recalculated when it was deleted.
		else
			RecalcGroupMembers( groupPK, l, true );//collect+clear the members before erasing, RecursiveUsers can't find them after.
		PurgeIdentity( groupPK, l );//after the recalc - the group is already out of Groups, so its acl rows are inert either way.
	}

	α Authorize::TestAddRoleMember( RolePK parent, RolePK child, SL sl )ε->void{
		THROW_IFX( parent==child, Exception(sl, ELogLevel::Debug, "Role cannot be a member of itself.") );
		flat_set<RolePK> visited;
		function<bool( RolePK,RolePK )> isChild = [&]( RolePK parent, RolePK child )->bool {
			auto children = visited.emplace( parent ).second ? Roles.find( parent ) : Roles.end();//visited guards cycles in existing data.
			if( children==Roles.end() )
				return false;
			for( PermissionRole member : children->second.Members ){
				if( member.index()==1 && (get<1>(member)==child || isChild(get<1>(member), child)) )
					return true;
			}
			return false;
		};
		std::shared_lock _{ Mutex };
		THROW_IFX( isChild(child, parent), Exception(sl, ELogLevel::Debug, "Role '{}' cannot be a member of '{}' because it is a ancester.", child, parent) );
	}
	α Authorize::AddRolePermission( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, const jobject& jResource )ι->void{
		ul l{ Mutex };
		if( auto permssion = Permissions.find(member); permssion!=Permissions.end() ){
			permssion->second.Allowed = allowed;
			permssion->second.Denied = denied;
		}
		else{
			Resource resource{ jResource };
			if( auto p = FindResource(resource, l); p )
				Permissions.emplace( member, Permission{member, p->PK, allowed, denied} );
			else if( !resource.PK ){
				CRITICAL( "[{}]Resource '{}' not found for role permission.", member, resource.Target );
			}else{ //new resource
				auto& saved = Resources.emplace( resource.PK, move(resource) ).first->second;
				ASSERT( saved.Schema.size() && saved.Target.size() );
				SchemaResources[saved.Schema][saved.Target][saved.Criteria] = saved.PK;
				Permissions.emplace( member, Permission{member, saved.PK, allowed, denied} );
			}
		}
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

	α Authorize::RemoveRoleChildren( 	RolePK rolePK, flat_set<PermissionRightsPK> toRemove )ι->void{
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
				if( member==std::visit([](auto id)->PermissionRightsPK{return id;}, *p) ) {
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
		const PermissionRole member{ std::in_place_index<1>, rolePK };
		for( auto acl=Acl.begin(); acl!=Acl.end(); )//Acl is keyed by identity, so a role has to be swept by value.
			acl = acl->second==member ? Acl.erase( acl ) : std::next( acl );
		for( auto role=Roles.begin(); role!=Roles.end(); ++role )
			role->second.Members.erase( member );
		if( !deleted )
			Recalc( l );
	}

	α Authorize::AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void{
		flat_set<GroupPK> visitedGroups;
		AddPermission( identityPK, permissionRole, users, visitedGroups, l );
	}
	α Authorize::AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, flat_set<GroupPK>& visitedGroups, const ul& l )ι->void{
		if( auto pkUser = identityPK.IsUser() ? Users.find(identityPK.UserPK()) : Users.end(); pkUser!=Users.end() ){
			if( !users.empty() && !users.contains(pkUser->first) )
				return;
			flat_set<RolePK> visitedRoles;
			AddUserPermissions( pkUser->second, permissionRole, visitedRoles );
		}
		else if( auto group = identityPK.IsUser() ? Groups.end() : Groups.find(identityPK.GroupPK()); group!=Groups.end() && !group->second.IsDeleted && visitedGroups.emplace(group->first).second ){//deleted groups don't propagate, mirrors RecursiveUsers.
			for( auto member : group->second.Members )
				AddPermission( member, permissionRole, users, visitedGroups, l );//user
		}
	}
	α Authorize::AddUserPermissions( User& user, PermissionRole permissionRole, flat_set<RolePK>& visitedRoles )ι->void{
		if( auto p = permissionRole.index()==0 ? Permissions.find(get<0>(permissionRole)) : Permissions.end(); p!=Permissions.end() )
			user += p->second;
		else if( auto rolePermissions = permissionRole.index()==1 ? Roles.find(get<1>(permissionRole)) : Roles.end(); rolePermissions!=Roles.end() && !rolePermissions->second.IsDeleted && visitedRoles.emplace(rolePermissions->first).second ){
			for( let& rolePermission : rolePermissions->second.Members )
				AddUserPermissions( user, rolePermission, visitedRoles );
		}
	}
	α Authorize::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		ul l{ Mutex };
		auto p = Permissions.find( permissionPK ); THROW_IF( p==Permissions.end(), "[{}]Permission not found", permissionPK );
		p->second.Update( allowed, denied );
		for( let& user : Users )
			user.second.UpdatePermission( permissionPK, allowed, denied );
	}
	α Authorize::Recalc( const ul& l )ι->void{
		for( let& user : Users )
			user.second.Clear();
		SetUserPermissions( {}, l );
	}
	α Authorize::SetUserPermissions( flat_set<UserPK>&& users, const ul& l )ι->void{
		for( let& [identityPK,permissionRole] : Acl )
			AddPermission( identityPK, permissionRole, users, l );
	}
}