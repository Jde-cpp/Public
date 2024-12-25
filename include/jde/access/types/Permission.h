#pragma once
#include <jde/access/usings.h>
//#include "../accessInternal.h"
//#include "User.h"
//#include "Resource.h"

namespace Jde::DB{ struct AppSchema; struct Table; }

namespace Jde::Access{
	struct Permission final{
		Permission( const jobject& j )ι;
		Permission( PermissionPK pk, Access::ResourcePK resourcePK, ERights Allowed, ERights Denied )ι;

		α Update( optional<ERights> allowed, optional<ERights> denied )ι->void;
		PermissionPK PK;
		Access::ResourcePK ResourcePK;
		ERights Allowed;
		ERights Denied;
	};
}
