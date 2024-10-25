#pragma once
#include <jde/access/usings.h>

namespace Jde::DB{ struct AppSchema; struct Table; }

namespace Jde::Access{
	using PermissionPK=uint16;
	struct Permission final{
		PermissionPK PK;
		ERights Allowed;
		ERights Denied;
	};
}