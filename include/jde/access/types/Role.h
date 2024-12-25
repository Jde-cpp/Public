#pragma once
#include <jde/access/usings.h>
//#include "../accessInternal.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	using PermissionRightPK=uint32;

	struct RoleRights final{
		Access::RolePK RolePK;
		Access::PermissionPK PermissionPK;
	};

	struct Role final{
		Role( RolePK rolePK, bool isDeleted  )ι:PK{rolePK}, IsDeleted{isDeleted}{}
		Role( const jobject& j )ι;
		RolePK PK;
		bool IsDeleted;
		flat_set<PermissionRole> Members;
	};
}