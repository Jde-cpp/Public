#pragma once
//#include <jde/framework/coroutine/TaskOld.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/access/usings.h>
#include "Permission.h"
#include <jde/ql/GraphQLHook.h>

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	using ResourcePK=uint16;
namespace Resources{
	α Sync()ε->void;
}

	struct Resource{
		Access::ResourcePK PK;
		string Schema;
		string Target;
		string Filter;
		flat_map<PermissionPK,Permission> Permissions;
	};

	struct ResourceLoadAwait final : TAwait<flat_map<ResourcePK,Resource>>{
		ResourceLoadAwait( sp<DB::AppSchema> schema, vector<AppPK> appPKs )ι;
		const vector<AppPK> AppPKs;
	private:
		α Suspend()ι->void override;
		sp<DB::AppSchema> _schema;
	};
}