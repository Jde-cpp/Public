#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/RowAwait.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLHook.h>
#include "Permission.h"


namespace Jde::DB{ struct IRow; struct AppSchema; }
namespace Jde::Access{
namespace Resources{ α Sync()ε->void; }

	struct Resource{
		Resource()ι=default;
		Resource( ResourcePK pk, const jobject& j )ι;
		Resource( DB::IRow& row )ι;
		Resource( const jobject& j )ι;
		Access::ResourcePK PK;
		string Schema;
		string Target;
		string Filter;
		optional<TimePoint> Deleted;
	};
	struct ResourcePermissions{ flat_map<ResourcePK,Resource> Resources; flat_map<PermissionPK,Permission> Permissions; };
	struct ResourceLoadAwait final : TAwait<ResourcePermissions>{
		ResourceLoadAwait( sp<QL::IQL> qlServer, vector<string> schemaNames, UserPK executer )ι:
			_executer{executer}, _qlServer{qlServer}, _schemaNames{schemaNames}{};
	private:
		α Suspend()ι->void{ Load(); }
		α Load()ι->QL::QLAwait::Task;

		UserPK _executer;
		sp<QL::IQL> _qlServer;
		vector<string> _schemaNames;
	};
}