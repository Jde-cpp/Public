#include "AclLoadAwait.h"
#include <jde/ql/IQL.h>

#define let const auto
namespace Jde::Access{
	α AclLoadAwait::Load()ι->QL::QLAwait<jarray>::Task{
		try{
			flat_multimap<IdentityPK,PermissionRole> y;
			let values = co_await *_qlServer->QueryArray( "acl{ identity{id isGroup} permission{id isRole} }", {}, _executer );
			for( let& value : values ){
				let acl = Json::AsObject(value);
				let groupUserPK = Json::AsNumber<IdentityPK::Type>( acl, "identity/id" );
				let identityPK = Json::AsBool(acl, "identity/isGroup") ? IdentityPK{ GroupPK{groupUserPK} } : IdentityPK{ UserPK{groupUserPK} };
				let permissionRolePK = Json::AsNumber<PermissionPK>( acl, "permission/id" );
				let permissionRole = Json::AsBool(acl, "permission/isRole") ? PermissionRole{ std::in_place_index<1>, permissionRolePK } : PermissionRole{ std::in_place_index<0>, permissionRolePK };
				y.emplace( identityPK, permissionRole );
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}