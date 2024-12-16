#include "AclLoadAwait.h"

#define let const auto
namespace Jde::Access{
	α AclLoadAwait::Load()ι->QL::QLAwait::Task{
		try{
			flat_multimap<IdentityPK,PermissionRole> y;
			let values = co_await _qlServer->Query( "acl{ identity{id is_group} permission{id is_role} }", _executer );
			for( let& value : Json::AsArray(values) ){
				let acl = Json::AsObject(value);
				let groupUserPK = Json::AsNumber<IdentityPK::Type>( acl, "identity/id" );
				let identityPK = Json::AsBool(acl, "identity/is_group") ? IdentityPK{ GroupPK{groupUserPK} } : IdentityPK{ UserPK{groupUserPK} };
				let permissionRolePK = Json::AsNumber<PermissionPK>( acl, "permission/id" );
				let permissionRole = Json::AsBool(acl, "permission/is_role") ? PermissionRole{ std::in_place_index<1>, permissionRolePK } : PermissionRole{ std::in_place_index<0>, permissionRolePK };
				y.emplace( identityPK, permissionRole );
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	/*
	α AclLoadAwait::Load()ι->QL::QLAwait::Task{
		try{
			let aclTable = schema->GetTablePtr( "acl" );
			let permissionsTable = schema->GetTablePtr( "permissions" );
			let identitiesTable = schema->GetTablePtr( "identities" );
			let aclIdentityFK = aclTable->GetColumnPtr("identity_id");
			DB::SelectClause select{ {aclIdentityFK, aclTable->GetColumnPtr("permission_id"), permissionsTable->GetColumnPtr("is_role"), identitiesTable->GetColumnPtr("is_group")} };
			DB::FromClause from{ DB::Join{aclTable->GetColumnPtr("permission_id"), permissionsTable->GetColumnPtr("permission_id"), true} };
			from+=DB::Join{aclIdentityFK, identitiesTable->GetPK(), true};
			let rows = co_await DS()->SelectCo( DB::Statement{move(select), move(from), {}}.Move() );
			flat_multimap<IdentityPK,PermissionRole> acl;
			for( let& row : rows ){
				let identityPK = row->Get<IdentityPK::Type>(0);
				let groupUserPK = row->GetBit(3) ? IdentityPK{ GroupPK{identityPK} } : IdentityPK{UserPK{identityPK}};
				let permissionPK = row->Get<PermissionPK>(1);
				let permissionRole = row->GetBit(2) ? PermissionRole{std::in_place_index<1>, permissionPK} : PermissionRole{std::in_place_index<0>, RolePK{permissionPK}};
				acl.emplace( groupUserPK, permissionRole );
			}
			await.Resume( move(acl) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
*/
}