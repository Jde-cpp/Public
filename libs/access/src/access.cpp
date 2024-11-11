#include <jde/access/access.h>
#include <jde/access/IAcl.h>
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

#define let const auto

namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α ToString( ERights x )ι->sv{ return FromEnum<ERights>( RightsStrings, x ); }
	α ToProviderType( sv x )ι->EProviderType{ return ToEnum<EProviderType>( ProviderTypeStrings, x ).value_or(EProviderType::None); }
	α ToString( EProviderType x )ι->sv{ return FromEnum<EProviderType>( ProviderTypeStrings, x ); }

	static sp<DB::AppSchema> _schema;
	α GetTable( str name )ε->sp<DB::View>{ return _schema->GetViewPtr( name ); }
	α GetSchema()ε->sp<DB::AppSchema>{ return _schema; }

	struct Authorize : IAcl{
		Authorize()ε{}
		α Test( ERights access, str resource, UserPK userPK )ε->void override;
		concurrent_flat_map<string, flat_map<string,Access::ResourcePK>> AppResources;//string=schema
		concurrent_flat_map<ResourcePK,Resource> Resources;
		flat_multimap<RolePK,PermissionPK> Roles;
		flat_multimap<IdentityPK,PermissionPK> Acl;
		sp<DB::AppSchema> Schema;
	private:
		concurrent_flat_map<UserPK,AllowedDisallowed> _users;
	};
	sp<Authorize> _authorize = ms<Authorize>();

	α Authorize::Test( ERights access, str resourceName, UserPK userId )ε->void{
		optional<ResourcePK> resourcePK;
		if( !AppResources.cvisit(SchemaName,[&](auto& kv){
			if( let p = kv.second.find(resourceName); p!=kv.second.end() )
				resourcePK = p->second;
		}) || !resourcePK ){
			return;//not enabled
		}
		AllowedDisallowed configured;
		if( !_users.cvisit( userId, [&](let& kv){configured = kv.second;}) )
			THROW( "[{}]User not found.", userId );
		THROW_IF( !empty(configured.Denied & access), "[{}]User denied access to '{}'.", userId, resourceName );
		THROW_IF( empty(configured.Allowed & access), "[{}]User does not have access to '{}'.", userId, resourceName );
	}

/*	α LoadPermissions( const concurrent_flat_map<UserPK,User>& users, ConfigureAwait& await )->UserLoadAwait::Task{
		let permissions = co_await PermissionLoadAwait( _schema );
	}*/
	ConfigureAwait::ConfigureAwait()ι// vec<AppPK> appPKs )ι:
		//AppPKs{appPKs}
	{}

	α loadAcl( ConfigureAwait& await )->RoleLoadAwait::Task{
		try{
			let acl = co_await AclLoadAwait{ _schema };
			_authorize->Acl = move(acl);
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
  α loadRoles( ConfigureAwait& await )->RoleLoadAwait::Task{
		try{
			let roles = co_await RoleLoadAwait{ _schema };
			_authorize->Roles = move(roles);
			loadAcl( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
	α loadResources( ConfigureAwait& await )->ResourceLoadAwait::Task{
		try{
			flat_map<ResourcePK,Resource> resources = co_await ResourceLoadAwait{ _schema };
			for( let& [pk, resource] : resources ){
				if( resource.Filter.empty() ){
					_authorize->AppResources.try_emplace_or_visit( resource.Schema, flat_map<string,ResourcePK>{{resource.Target, pk}}, [&](auto& kv){
					 	kv.second.emplace( resource.Target, pk );
					});
				}
				_authorize->Resources.emplace( pk, move(resource) );
			}
			loadRoles( await );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
	α loadUsers( ConfigureAwait& await )->UserLoadAwait::Task{
		try{
			let users = co_await UserLoadAwait{ _schema };
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
		QL::Hook::Add( mu<Access::UserGraphQL>() );
		QL::Hook::Add( mu<Access::GroupGraphQL>() );
		return {};
		//schema->GetTable( "um_users" )->Load();
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
			auto userId = p ? *p : 0;
			if( !userId ){
				if( !opcServer.size() )
					parameters.push_back( {move(opcServer)} );
				_schema->DS()->ExecuteProc( "um_user_insert_login(?,?,?)", move(parameters), [&userId](let& row){userId=row.GetUInt32(0);} );
			}
			h.promise().SetResult( mu<UserPK>(userId) );
		}
		catch( IException& e ){
			h.promise().SetResult( move(e) );
		}
		h.resume();
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
	α AssignUser( UserPK userId, const flat_set<Access::GroupPK>& groupIds )ι->void{
		for( let groupId : groupIds ){
			let pGroupRoles = _groupRoles.find(groupId); if( pGroupRoles==_groupRoles.end() ) continue;
			for( let roleId : pGroupRoles->second ){
				sl _{_rolePermissionMutex};
				let pRolePermissions = _rolePermissions.find( roleId ); if( pRolePermissions==_rolePermissions.end() ) continue;
				for( let& [permissionPK, access] : pRolePermissions->second )
					_userAccess.try_emplace( userId ).first->second.try_emplace( permissionPK, access );
			}
		}
	}
	*/
/*	α AssignUserGroups( UserPK userId=0 )ε->void{
		ostringstream sql{ "select member_id, entity_id from um_groups g join um_entities e on g.member_id=e.id and e.deleted is null", std::ios::ate };
		vector<DB::Value> params;
		if( userId ){
			sql << " where user_id=?";//TODO - no user_id column
			params.push_back( DB::Value{userId} );
		}
		DB::DataSource().Select( sql.str(), [&](let& r){
			unique_lock l{_userGroupMutex};
			_userGroups.try_emplace(r.GetUInt32(0)).first->second.emplace( r.GetUInt(1) );
		}, params );
	}
	α SetRolePermissions()ε->void{
		unique_lock _{ _rolePermissionMutex };
		_rolePermissions.clear();
		DB::DataSource().Select( "select permission_id, role_id, right_id from um_role_permissions p join um_roles r on p.role_id=r.id where r.deleted is null", [&](const DB::IRow& r){_rolePermissions.try_emplace(r.GetUInt(1)).first->second.emplace( r.GetUInt(0), (Access::EAccess)r.GetUInt(2) );} );
	}*/
/*	α Access::Configure( sp<DB::AppSchema> schema )ε->void{
		let pApis = _ds->SelectEnumSync<uint,string>( "um_apis" );
		auto pId = FindKey( *pApis, "UM" ); THROW_IF( !pId, "no user management in api table." );
		let umPermissionId = _ds->Scaler<uint>( "select id from um_permissions where api_id=? and name is null", {*pId} ).value_or(0); THROW_IF( umPermissionId==0, "no user management permission." );
		for( let& table : schema.Tables )
			_tablePermissions.try_emplace( table.first, umPermissionId );

		AssignUserGroups();
		SetRolePermissions();
		_ds->Select( "select entity_id, role_id from um_entity_roles er join um_entities e on er.entity_id=e.id and e.deleted is null join um_roles r on er.role_id=r.id and r.deleted is null", [&](let& r){_groupRoles.try_emplace(r.GetUInt(0)).first->second.emplace( r.GetUInt(1) );} );
		for( let& [userId,groupIds] : _userGroups )
			AssignUser( userId, groupIds );
	}
	α IsTarget( sv url )ι{ return string{url}.starts_with(Access::AccessSettings().Target); }
	α Access::TestAccess( EAccess access, UserPK userId, PermissionPK permissionId )ε->void{
		sl _{ _userAccessMutex };
		let pUser = _userAccess.find( userId ); THROW_IF( pUser==_userAccess.end(), "User '{}' not found.", userId );
		let pAccess = pUser->second.find( permissionId ); THROW_IF( pAccess==pUser->second.end(), "User '{}' does not have api '{}' access.", userId, permissionId );
		THROW_IF( (pAccess->second & access)==EAccess::None, "User '{}' api '{}' access is limited to:  '{}'. requested:  '{}'.", userId, permissionId, (uint8)pAccess->second, (uint8)access );
	}
	α TestAccess( str tableName, UserPK userId, Access::EAccess access )ε->void{
		let pTable = _tablePermissions.find( tableName ); THROW_IF( pTable==_tablePermissions.end(), "Could not find table '{}'", tableName );
		TestAccess( access, userId, pTable->second );
	}
	α Access::TestRead( str tableName, UserPK userId )ε->void{
		Jde::TestAccess( tableName, userId, EAccess::Read );
	}

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
			//	let access = (Access::EAccess)DB::DataSource().Scaler<uint>( "select right_id from um_role_permissions where role_id=? and permission_id=?", std::vector<DB::Value>{roleId, permissionId} ).value_or( 0 );
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
				for( let& [userId,userGroupIds] : _userGroups )
				{
					for( let permissionGroupId : groupIds )
					{
						if( userGroupIds.find(permissionGroupId)!=userGroupIds.end() )
						{
							AssignUser( userId, userGroupIds );
							break;
						}
					}
				}
			}
		}
	}
}

#pragma warning(disable:4100)
	α IAcl::TestPurge( uint pk, UserPK userId, SL sl )ε->void{
		THROW_IFSL( !CanPurge(pk, userId), "Access to purge record denied" );
	}
	*/
}
