#include <jde/access/access.h>
#include <jde/access/IAcl.h>
#include <jde/access/hooks/RoleHook.h>
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include <jde/db/Database.h>
#include "types/Acl.h"
#include "types/IdentityGroup.h"
#include "types/Resource.h"
#include "types/Role.h"
#include "types/User.h"
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include "../../../../Framework/source/DateTime.h"

#define let const auto

namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α ToProviderType( sv x )ι->EProviderType{ return ToEnum<EProviderType>( ProviderTypeStrings, x ).value_or(EProviderType::None); }
	α ToString( EProviderType x )ι->sv{ return FromEnum<EProviderType>( ProviderTypeStrings, x ); }

	static sp<DB::AppSchema> _schema;
	α GetTable( str name )ε->sp<DB::View>{ return _schema->GetViewPtr( name ); }
	α GetSchema()ε->sp<DB::AppSchema>{ return _schema; }
	struct Authorize : IAcl{
		Authorize()ε{}
		α Test( str schemaName, str resourceName, ERights rights, UserPK userPK, SRCE )ε->void override;
		α CalculateUsers( flat_set<UserPK>&& users, const ul& l )ι->void;
		α RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove=false )ι->void;
		α Recalc( const ul& l )ι->void;
		α AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void;
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α GroupUsers( GroupPK groupPK, const ul& l, bool clear=false )ι->flat_set<UserPK>;

		mutable std::shared_mutex Mutex;
		flat_map<string, flat_map<string,Access::ResourcePK>> SchemaResources;//schemaName, resourceJsonName, resourcePK -- active only
		flat_map<ResourcePK,Resource> Resources; //includes inactive resources.
		flat_map<UserPK,User> Users;

		flat_map<PermissionPK,Permission> Permissions;
		flat_multimap<GroupPK,UserPK> GroupMembers;
		flat_map<RolePK,flat_set<PermissionRole>> RoleMembers;
		flat_multimap<IdentityPK,PermissionRole> Acl;
	};

	sp<Authorize> _authorize = ms<Authorize>();

	α Authorize::Test( str schemaName, str resourceName, ERights rights, UserPK userPK, SL sl )ε->void{
		optional<ResourcePK> resourcePK;
		Jde::sl _{Mutex};
		if( auto schemaResorces = SchemaResources.find(schemaName); schemaResorces!=SchemaResources.end() )
			resourcePK = Find( schemaResorces->second, resourceName );
		if( !resourcePK )//not enabled
			return;

		if( auto user = Users.find(userPK); user!=Users.end() ){
			THROW_IFSL( user->second.Deleted, "[{}]User is deleted.", userPK );
			let configured = user->second.ResourceRights( *resourcePK );
			THROW_IFSL( !empty(configured.Denied & rights), "[{}]User denied '{}' access to '{}'.", userPK, ToString(rights), resourceName );
			THROW_IFSL( empty(configured.Allowed & rights), "[{}]User does not have '{}' access to '{}'.", userPK, ToString(rights), resourceName );
		}
		else
			throw Exception{ sl, ELogLevel::Debug, "[{}]User not found.", userPK };

	}

	ConfigureAwait::ConfigureAwait()ι
	{}

	Ω loadAcl( ConfigureAwait& await )->AclLoadAwait::Task{
		try{
			let acl = co_await AclLoadAwait{ _schema };
			ul l{ _authorize->Mutex };
			_authorize->Acl = move( acl );
			_authorize->CalculateUsers( {}, l );
			await.Resume();
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
  Ω loadRoles( ConfigureAwait& await )ι->RoleLoadAwait::Task{
		try{
			auto roles = co_await RoleLoadAwait{ _schema };
			ul l{ _authorize->Mutex };
			_authorize->RoleMembers = move( roles );
			l.unlock();
			loadAcl( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
	Ω loadResources( ConfigureAwait& await )ι->ResourceLoadAwait::Task{
		try{
			auto loaded = co_await ResourceLoadAwait{ _schema };
			ul l{ _authorize->Mutex };
			_authorize->Permissions = move( loaded.Permissions );
			auto& namePK = _authorize->SchemaResources.emplace( _schema->Name, flat_map<string,ResourcePK>{} ).first->second;
			for( let& [pk, resource] : loaded.Resources ){
				if( resource.Filter.empty() && !resource.Deleted )
					namePK.emplace( resource.Target, pk );
				_authorize->Resources.emplace( pk, move(resource) );
			}
			l.unlock();
			loadRoles( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
	α loadUsers( ConfigureAwait& await )ι->UserLoadAwait::Task{
		try{
			auto identities = co_await UserLoadAwait{ _schema };
			ul l{ _authorize->Mutex };
			_authorize->Users = std::move( identities.Users );
			_authorize->GroupMembers = std::move( identities.GroupMembers );
			l.unlock();
			loadResources( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ConfigureAwait::Suspend()ι->void{
		loadUsers( *this );
	};
}

namespace Jde{
	α Access::LocalAcl()ι->sp<IAcl>{
		return _authorize;
	}

	α Access::Configure( sp<DB::AppSchema> schema )ε->ConfigureAwait{
		_schema = schema;
		Resources::Sync();
		QL::Hook::Add( mu<Access::AclHook>() );
		QL::Hook::Add( mu<Access::UserGraphQL>() );
		QL::Hook::Add( mu<Access::GroupGraphQL>() );
		QL::Hook::Add( mu<Access::RoleHook>() );
		return {};
	}
namespace Access{
	α AuthTask( string&& loginName, uint providerId, string&& opcServer, HCoroutine h )ι->Task{
		let opcServerParam = opcServer.size() ? DB::Value{opcServer} : DB::Value{nullptr};
		vector<DB::Value> parameters = { {move(loginName)}, {providerId} };
		if( opcServer.size() )
			parameters.push_back( {opcServer} );
		let sql = Ƒ( "select e.id from um_entities e join um_users u on e.id=u.entity_id join um_providers p on p.id=e.provider_id where u.login_name=? and p.id=? and p.target{}", opcServer.size() ? "=?" : " is null" );
		try{
			auto task = _schema->DS()->ScalerCo<UserPK>( string{sql}, parameters );
			auto p = (co_await task).UP<UserPK>(); //gcc compile issue
			auto userPK = p ? *p : 0;
			if( !userPK ){
				if( !opcServer.size() )
					parameters.push_back( {move(opcServer)} );
				_schema->DS()->ExecuteProc( "um_user_insert_login(?,?,?)", move(parameters), [&userPK](let& row){userPK=row.GetUInt32(0);} );
			}
			h.promise().SetResult( mu<UserPK>(userPK) );
		}
		catch( IException& e ){
			h.promise().SetResult( move(e) );
		}
		h.resume();
	}
	α AddAcl( IdentityPK identityPK, uint permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void{
		ul l{ _authorize->Mutex };
		const PermissionRole permissionRole{ std::in_place_index<0>, permissionPK };
		_authorize->Acl.emplace( identityPK, permissionRole );
		const Permission permission{ permissionPK, resourcePK, allowed, denied };
		_authorize->Permissions.emplace( permissionPK, permission );
		if( auto user = _authorize->Users.find(identityPK); user!=_authorize->Users.end() )
			user->second += permission;
		else
			_authorize->RecalcGroupMembers( identityPK, l );
	}
	α AddAcl( IdentityPK identityPK, RolePK rolePK )ι->void{
		ul l{ _authorize->Mutex };
		_authorize->Acl.emplace( identityPK, PermissionRole{std::in_place_index<1>, rolePK} );
		if( auto user = _authorize->Users.find(identityPK); user!=_authorize->Users.end() )
			_authorize->AddPermission( identityPK, PermissionRole{std::in_place_index<1>, rolePK}, {identityPK}, l );
		else
			_authorize->RecalcGroupMembers( identityPK, l );
	}

	α CreateUser( IdentityPK identityPK )ι->void{
		ul _{_authorize->Mutex};
		_authorize->Users.emplace( identityPK, User{identityPK, false} );
	}
	α DeleteUser( IdentityPK identityPK )ι->void{
		ul _{_authorize->Mutex};
		if( auto p = _authorize->Users.find(identityPK); p!=_authorize->Users.end() )
			p->second.Deleted = true;
	}
	α RestoreUser( IdentityPK identityPK )ι->void{
		ul _{_authorize->Mutex};
		if( auto p = _authorize->Users.find(identityPK); p!=_authorize->Users.end() )
			p->second.Deleted = false;
	}
	α DeleteGroup( IdentityPK identityPK )ι->void{
		ul l{ _authorize->Mutex };
		_authorize->RecalcGroupMembers( identityPK, l, true );
	}

	α	DeleteRole( RolePK rolePK )ι->void{
		ul l{ _authorize->Mutex };
		auto roleIt = _authorize->RoleMembers.equal_range( rolePK );
		for( auto p = roleIt.first; p!=roleIt.second; ++p )
			_authorize->RoleMembers.erase( p );
		if( roleIt.first!=roleIt.second )
			_authorize->Recalc( l );//not sure a better way than recalc all users.
	}
	α RestoreRole( RolePK rolePK, flat_set<PermissionRole> members )ι->void{
		if( !members.size() )
			return;
		ul l{ _authorize->Mutex };
		_authorize->RoleMembers[rolePK] = members;
		_authorize->Recalc( l );
	}
	α AddToRole( RolePK rolePK, flat_set<PermissionRole> members )ι->void{
		RestoreRole( rolePK, move(members) );
	}
	α RemoveFromRole(	RolePK rolePK, flat_set<PermissionIdentityPK> toRemove )ι->void{
		if( !toRemove.size() )
			return;
		ul l{ _authorize->Mutex };
		for( let& member : toRemove ){
			auto roleMembers = _authorize->RoleMembers.find( rolePK );
			if( roleMembers==_authorize->RoleMembers.end() )
				continue;
			auto& members = roleMembers->second;
			for( auto p = members.begin(); p!=members.end(); ++p ){
				if( member==std::visit([](auto id)->PermissionIdentityPK{return id;}, *p) ){
					members.erase( p );
					break;
				}
			}
		}
		_authorize->Recalc( l );
	}
	α Authorize::Recalc( const ul& l )ι->void{
		for( auto& user : Users )
			user.second.Clear();
		CalculateUsers( {}, l );
	}


	α Authorize::GroupUsers( GroupPK groupPK, const ul& l, bool clear )ι->flat_set<UserPK>{
		flat_set<UserPK> users;
		auto groupIt = GroupMembers.equal_range( groupPK );
		for( auto group = groupIt.first; group!=groupIt.second; ++group ){
			if( auto pkUser = Users.find(group->second); pkUser!=Users.end() ){
				users.emplace( group->second );
				if( clear )
					pkUser->second.Clear();
			}
			else{
				let groupUsers = GroupUsers( group->second, l );
				users.insert( groupUsers.begin(), groupUsers.end() );
			}
		}
		return users;
	}

	α Authorize::RecalcGroupMembers( GroupPK groupPK, const ul& l, bool remove )ι->void{
		auto users = GroupUsers( groupPK, l, true );
		if( remove )
			GroupMembers.erase( groupPK );
		if( users.size() )
			CalculateUsers( move(users), l );
	}

	α RestoreGroup( IdentityPK identityPK, flat_set<IdentityPK>&& members )ι->void{
		if( !members.size() )
			return;
		ul l{ _authorize->Mutex };

		for( let& member : members )
			_authorize->GroupMembers.emplace( identityPK, member );

		_authorize->RecalcGroupMembers( identityPK, l );
	}

	α AddToGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void{
		ul l{ _authorize->Mutex };
		flat_set<UserPK> users;
		for( let& member : members ){
			_authorize->GroupMembers.emplace( groupPK, member );
			if( auto pkUser = _authorize->Users.find(member); pkUser!=_authorize->Users.end() ){
				pkUser->second.Clear();
				users.emplace( member );
			}
			else{
				let groupUsers = _authorize->GroupUsers( member, l, true );
				for( let user : groupUsers )
					users.emplace( user );
			}
		}
		if( users.size() )
			_authorize->CalculateUsers( move(users), l );
	}
	α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void{
		ul l{ _authorize->Mutex };
		flat_set<UserPK> users;
		for( let& member : members ){
			auto groupMembers = _authorize->GroupMembers.equal_range( groupPK );
			for( auto p = groupMembers.first; p!=groupMembers.second; ++p ){
				if( p->second!=member )
					continue;
				_authorize->GroupMembers.erase( p );
				if( auto pkUser = _authorize->Users.find(member); pkUser!=_authorize->Users.end() ){
					pkUser->second.Clear();
					users.emplace( member );
				}
				else{
					let groupUsers = _authorize->GroupUsers( member, l, true );
					for( let user : groupUsers )
						users.emplace( user );
				}
				break;
			}
		}
		if( users.size() )
			_authorize->CalculateUsers( move(users), l );
	}

	α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		_authorize->UpdatePermission( permissionPK, allowed, denied );
	}
	α Authorize::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		ul l{Mutex};
		auto p = Permissions.find(permissionPK); THROW_IF( p==Permissions.end(), "[{}]Permission not found", permissionPK );
		p->second.Update( allowed, denied );
		for( auto& user : Users )
			user.second.UpdatePermission( permissionPK, allowed, denied );
	}
	α CreateResource( Resource&& resource )ε->void{
		ul _{_authorize->Mutex};
		_authorize->Resources[resource.PK] = move(resource);
	}
	α UpdateResourceDeleted( str schemaName, const jobject& args, bool restored )ε->void{
		ul _{_authorize->Mutex};
		let pk = Json::FindNumber<ResourcePK>( args, "id" );
		let target = Json::FindSV( args, "target" );
		auto pkResource = find_if( _authorize->Resources, [&](auto&& pkResource){
			let& r = pkResource.second;
			return (pk && *pk==r.PK) || ( r.Schema==schemaName && target && *target==r.Target );
		} );
		THROW_IF( pkResource==_authorize->Resources.end(), "Resource not found schema:{}, args:{}", schemaName, serialize(args) );
		auto& resource = pkResource->second;

		resource.Deleted = restored ? optional<DB::DBTimePoint>{} : DB::DBClock::now();
		if( auto schemaResources = resource.Filter.empty() ? _authorize->SchemaResources.find( schemaName ) : _authorize->SchemaResources.end(); schemaResources!=_authorize->SchemaResources.end() ){
			if( restored )
				schemaResources->second.emplace( resource.Target, resource.PK );
			else
				schemaResources->second.erase( resource.Target );
		};
	}

	α Authorize::AddPermission( IdentityPK identityPK, PermissionRole permissionRole, const flat_set<UserPK>& users, const ul& l )ι->void{
		if( auto pkUser = Users.find(identityPK); pkUser!=Users.end() ){
			if( !users.empty() && !users.contains(identityPK) )
				return;
			if( auto p = permissionRole.index()==0 ? Permissions.find(get<0>(permissionRole)) : Permissions.end(); p!=Permissions.end() )
				pkUser->second += p->second;
			else if( auto rolePermissions = RoleMembers.find(get<1>(permissionRole)); rolePermissions!=RoleMembers.end() ){
				for( let& rolePermission : rolePermissions->second )
					AddPermission( identityPK, rolePermission, users, l );
			}
		}
		else{
			auto groupIt = GroupMembers.equal_range( identityPK );
			for( auto group = groupIt.first; group!=groupIt.second; ++group )
				AddPermission( group->second, permissionRole, users, l );
		}
	}
	α Authorize::CalculateUsers( flat_set<UserPK>&& users, const ul& l )ι->void{
		for( let& [identityPK,permissionRole] : Acl )
			AddPermission( identityPK, permissionRole, users, l );
	}
}
	α Access::Authenticate( string loginName, uint providerId, string opcServer, SL sl )ι->AsyncAwait{
		//auto f = [l=move(loginName), type, o=move(opcServer)](HCoroutine h)mutable{ LoginTask(move(l), type, move(o), move(h)); };
		return AsyncAwait{ [l=move(loginName), providerId, o=move(opcServer)](HCoroutine h)mutable{ return AuthTask(move(l), providerId, move(o), move(h)); }, sl, "Access::Authenticate" };
	}
	//AccessSettings _settings;
	//sp<DB::IDataSource> _ds;
	//up<QL::GraphQL> _ql;
/*	flat_map<string,PermissionPK> _tablePermissions;
	flat_map<UserPK,flat_set<GroupPK>> _userGroups; shared_mutex _userGroupMutex;
	flat_map<GroupPK,flat_set<RolePK>> _groupRoles; shared_mutex _groupRoleMutex;
	flat_map<RolePK,flat_map<PermissionPK,EAccess>> _rolePermissions; shared_mutex _rolePermissionMutex;
	flat_map<UserPK, flat_map<PermissionPK,EAccess>> _userAccess; shared_mutex _userAccessMutex;

	flat_map<sv, IAcl*> _authorizers; shared_mutex _authorizerMutex;
*/
	//GroupAuthorize _groupAuthorize;
}
namespace Jde{
/*	α Access::AddAuthorizer( Access::IAcl* p )ι->void{ _authorizers.emplace( p->TableName, p ); }//pre-main
	α Access::FindAuthorizer( sv table )ι->IAcl*{
		auto p = _authorizers.find( table );
		return p==_authorizers.end() ? nullptr : p->second;
	}
	α Access::Query( string q, UserPK userPK, SL /*sl* / )ε->json{
		return _ql->Query( move(q), userPK );
	}*/
	/*
	α AssignUser( UserPK userPK, const flat_set<Access::GroupPK>& groupIds )ι->void{
		for( let groupId : groupIds ){
			let pGroupRoles = _groupRoles.find(groupId); if( pGroupRoles==_groupRoles.end() ) continue;
			for( let roleId : pGroupRoles->second ){
				sl _{_rolePermissionMutex};
				let pRolePermissions = _rolePermissions.find( roleId ); if( pRolePermissions==_rolePermissions.end() ) continue;
				for( let& [permissionPK, access] : pRolePermissions->second )
					_userAccess.try_emplace( userPK ).first->second.try_emplace( permissionPK, access );
			}
		}
	}
	*/
/*	α AssignUserGroups( UserPK userPK=0 )ε->void{
		ostringstream sql{ "select member_id, entity_id from um_groups g join um_entities e on g.member_id=e.id and e.deleted is null", std::ios::ate };
		vector<DB::Value> params;
		if( userPK ){
			sql << " where user_id=?";//TODO - no user_id column
			params.push_back( DB::Value{userPK} );
		}
		DB::DataSource().Select( sql.str(), [&](let& r){
			unique_lock l{_userGroupMutex};
			_userGroups.try_emplace(r.GetUInt32(0)).first->second.emplace( r.GetUInt(1) );
		}, params );
	}
*/
/*
	α Access::ApplyMutation( const QL::MutationQL& m, UserPK id )ε->void{
		if( m.JsonName=="user" ){
			if( m.Type==QL::EMutationQL::Create /*|| m.Type==QL::EMutationQL::Restore* / ){
				{ unique_lock l{ _userAccessMutex }; _userAccess.try_emplace( id ); }
				// if( m.Type==QL::EMutationQL::Restore ){
				// 	AssignUserGroups( id );
				// 	{ unique_lock l{ _userGroupMutex }; AssignUser( id, _userGroups[id] ); }
				// }
			}
			else if( /*m.Type==QL::EMutationQL::Delete ||* / m.Type==QL::EMutationQL::Purge ){
				{ unique_lock l{ _userAccessMutex }; _userAccess.erase( (UserPK)id ); }
				{ unique_lock l{ _userGroupMutex }; _userGroups.erase( (UserPK)id ); }
			}
		}
		else if( m.JsonName=="groupRole" ){
			let groupId = m.InputParam( "groupId" ).get<uint>(); let roleId = m.InputParam( "roleId" ).get<uint>();
			unique_lock l{ _groupRoleMutex };
			if( m.Type==QL::EMutationQL::Add )
				_groupRoles.try_emplace(groupId).first->second.emplace( roleId );
			else if( let p = _groupRoles.find(groupId); m.Type==QL::EMutationQL::Remove && p != _groupRoles.end() )
				p->second.erase( std::remove_if(p->second.begin(), p->second.end(), [&](PK roleId2){return roleId2==roleId;}), p->second.end() );
		}
		else if( m.JsonName=="rolePermission" ){
			SetRolePermissions();
			let pRoleId = m.Args.find( "roleId" ); THROW_IF( pRoleId==m.Args.end(), "could not find roleId in mutation" );
			//uint roleId, permissionId;
			//if( m.Type==QL::EMutationQL::Update )
			//{
			//	let pPermissionId = m.Args.find("permissionId"); THROW_IF( pPermissionId==m.Args.end(), "could not find permissionId in mutation" );
			//	let pRoleId = m.Args.find("roleId"); THROW_IF( pRoleId==m.Args.end(), "could not find roleId in mutation" );
			//	roleId = pRoleId->get<uint>();
			//	permissionId = pPermissionId->get<uint>();
			//	let access = (Access::EAccess)DB::DataSource().Scaler<uint>( "select right_id from um_role_members where role_id=? and permission_id=?", std::vector<DB::Value>{roleId, permissionId} ).value_or( 0 );
			//	{
			//		unique_lock l{ _rolePermissionMutex };
			//		_rolePermissions.try_emplace( roleId ).first->second[permissionId] = (Jde::Access::EAccess)access;
			//	}
			//}
			//else
			//	SetRolePermissions();
			flat_set<uint> groupIds;
			{
				sl _{ _groupRoleMutex };
				for( let& [groupId,roleIds] : _groupRoles )
				{
					if( roleIds.find(*pRoleId)!=roleIds.end() )
						groupIds.emplace( groupId );
				}
			}
			flat_set<uint> userIds;
			{
				sl _{_userGroupMutex};
				for( let& [userPK,userGroupIds] : _userGroups )
				{
					for( let permissionGroupId : groupIds )
					{
						if( userGroupIds.find(permissionGroupId)!=userGroupIds.end() )
						{
							AssignUser( userPK, userGroupIds );
							break;
						}
					}
				}
			}
		}
	}
}

#pragma warning(disable:4100)
	α IAcl::TestPurge( uint pk, UserPK userPK, SL sl )ε->void{
		THROW_IFSL( !CanPurge(pk, userPK), "Access to purge record denied" );
	}
	*/
}
