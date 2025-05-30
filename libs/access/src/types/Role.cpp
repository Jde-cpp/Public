#include <jde/access/types/Role.h>

#define let const auto

namespace Jde::Access{
	Ω getMembers( const jobject& j )ι->flat_set<PermissionRole>{
		flat_set<PermissionRole> members;
		if( auto p = Json::FindArray(j, "permissionRights"); p ){
			for( let& value : *p )
				members.emplace( PermissionRole{std::in_place_index<0>, Json::AsNumber<PermissionPK>(Json::AsObject(value), "id")} );
		}
		if( auto p = Json::FindArray(j, "roles"); p ){
			for( let& value : *p )
				members.emplace( PermissionRole{std::in_place_index<1>, Json::AsNumber<RolePK>(Json::AsObject(value), "id")} );
		}
		return members;
	}

	Role::Role( const jobject& j )ι:
		PK{ Json::FindNumber<RolePK>(j, "id").value_or(0) },
		IsDeleted{ Json::FindBool(j, "deleted").value_or(false) },
		Members{ getMembers(j) }
	{}
}