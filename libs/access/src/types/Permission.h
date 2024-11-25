#pragma once
#include <jde/access/usings.h>
#include "User.h"
//#include "Resource.h"

namespace Jde::DB{ struct AppSchema; struct Table; }

namespace Jde::Access{
	using ResourcePK=uint16;
	struct Permission final{
		α Update( optional<ERights> allowed, optional<ERights> denied )ι->void;
		PermissionPK PK;
		Access::ResourcePK ResourcePK;
		ERights Allowed;
		ERights Denied;
	};
#ifdef notused
	struct PermissionLoadAwait final : TAwait<flat_map<PermissionPK,Permission>>{
		PermissionLoadAwait( sp<DB::AppSchema> schema )ι: _schema{schema}{};
	private:
		α Suspend()ι->void override{ Load(); }
		α Load()ι->DB::RowAwait::Task;
		sp<DB::AppSchema> _schema;
	};
#endif
}
