#include <jde/access/access.h>
#include <jde/access/IAcl.h>
#include <jde/access/hooks/RoleHook.h>
#include <jde/access/hooks/GroupHook.h>
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include "hooks/AclHook.h"
#include "types/Group.h"
#include "types/Resource.h"
#include "types/Role.h"
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include "../../../../Framework/source/DateTime.h"
#include "accessInternal.h"
#include "Authorize.h"


#define let const auto

namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α ToProviderType( sv x )ι->EProviderType{ return ToEnum<EProviderType>( ProviderTypeStrings, x ).value_or(EProviderType::None); }
	α ToString( EProviderType x )ι->sv{ return FromEnum<EProviderType>( ProviderTypeStrings, x ); }

	α AuthorizeAdmin( ResourcePK resourcePK, UserPK userPK, SL sl )ε->void{
		Authorizer().TestAdmin( resourcePK, userPK, sl );
	}
	α AuthorizeAdmin( str resource, UserPK userPK, SL sl )ε->void{
		Authorizer().TestAdmin( resource, userPK, sl );
	}
}

namespace Jde{
	α Access::LocalAcl()ι->sp<IAcl>{
		return AuthorizerPtr();
	}

	α Access::Configure( sp<DB::AppSchema> schema, sp<QL::IQL> qlServer, UserPK executer )ε->ConfigureAwait{
		SetSchema( schema );
		Resources::Sync();
		QL::Hook::Add( mu<Access::AclHook>() );
		QL::Hook::Add( mu<Access::UserGraphQL>() );
		QL::Hook::Add( mu<Access::GroupHook>() );
		QL::Hook::Add( mu<Access::RoleHook>() );
		vector<string> schemaNames{ schema->Name };
		return {qlServer, schemaNames, executer};
	}
namespace Access{
	α AddToRole( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, str resourceName )ι->void{
		ul l{ Authorizer().Mutex };
		if( auto permssion = Authorizer().Permissions.find(member); permssion!=Authorizer().Permissions.end() ){
			permssion->second.Allowed = allowed;
			permssion->second.Denied = denied;
		}
		else if( auto resource = find_if(Authorizer().Resources, [&](let& r){ return r.second.Target==resourceName;}); resource!=Authorizer().Resources.end() )
			Authorizer().Permissions.emplace( member, Permission{member, resource->first, allowed, denied} );

		auto role = Authorizer().Roles.find( rolePK );
		ASSERT( role!=Authorizer().Roles.end() );
		if( role!=Authorizer().Roles.end() )
			role->second.Members.emplace( PermissionRole{std::in_place_index<0>, member} );
		Authorizer().Recalc( l );
	}
	α AddToRole( RolePK parentRolePK, RolePK childRolePK )ι->void{
		ul l{ Authorizer().Mutex };
		auto role = Authorizer().Roles.find( parentRolePK );
		ASSERT( role!=Authorizer().Roles.end() );
		if( role!=Authorizer().Roles.end() )
			role->second.Members.emplace( PermissionRole{std::in_place_index<1>,childRolePK} );
		Authorizer().Recalc( l );
	}

	α RemoveFromRole(	RolePK rolePK, flat_set<PermissionIdentityPK> toRemove )ι->void{
		if( !toRemove.size() )
			return;
		ul l{ Authorizer().Mutex };
		for( let& member : toRemove ){
			auto role = Authorizer().Roles.find( rolePK );
			ASSERT( role!=Authorizer().Roles.end() );
			if( role==Authorizer().Roles.end() )
				continue;
			auto& members = role->second.Members;
			for( auto p = members.begin(); p!=members.end(); ++p ){
				if( member==std::visit([](auto id)->PermissionIdentityPK{return id;}, *p) ){
					members.erase( p );
					break;
				}
			}
		}
		Authorizer().Recalc( l );
	}
	α Authorize::Recalc( const ul& l )ι->void{
		for( auto& user : Users )
			user.second.Clear();
		CalculateUsers( {}, l );
	}
	α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		Authorizer().UpdatePermission( permissionPK, allowed, denied );
	}
	α Authorize::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		ul l{Mutex};
		auto p = Permissions.find(permissionPK); THROW_IF( p==Permissions.end(), "[{}]Permission not found", permissionPK );
		p->second.Update( allowed, denied );
		for( auto& user : Users )
			user.second.UpdatePermission( permissionPK, allowed, denied );
	}
	α CreateResource( Resource&& resource )ε->void{
		ul _{Authorizer().Mutex};
		Authorizer().Resources[resource.PK] = move(resource);
	}
	α UpdateResourceDeleted( str schemaName, const jobject& args, bool restored )ε->void{
		ul _{Authorizer().Mutex};
		let pk = Json::FindNumber<ResourcePK>( args, "id" );
		let target = Json::FindSV( args, "target" );
		auto pkResource = find_if( Authorizer().Resources, [&](auto&& pkResource){
			let& r = pkResource.second;
			return (pk && *pk==r.PK) || ( r.Schema==schemaName && target && *target==r.Target );
		} );
		THROW_IF( pkResource==Authorizer().Resources.end(), "Resource not found schema:{}, args:{}", schemaName, serialize(args) );
		auto& resource = pkResource->second;

		resource.Deleted = restored ? optional<DB::DBTimePoint>{} : DB::DBClock::now();
		if( auto schemaResources = resource.Filter.empty() ? Authorizer().SchemaResources.find( schemaName ) : Authorizer().SchemaResources.end(); schemaResources!=Authorizer().SchemaResources.end() ){
			if( restored )
				schemaResources->second.emplace( resource.Target, resource.PK );
			else
				schemaResources->second.erase( resource.Target );
		};
	}
}
	α Access::Authenticate( str loginName, uint providerId, str opcServer, SL sl )ι->AuthenticateAwait{
		return AuthenticateAwait{ loginName, providerId, opcServer, sl };
	}
}