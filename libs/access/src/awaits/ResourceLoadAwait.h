#pragma once
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Permission.h>
#include <jde/access/types/Resource.h>

namespace Jde::DB{ struct IRow; struct AppSchema; }
namespace Jde::QL{ struct IQL; }
namespace Jde::Access{
	namespace Resources{
		α Sync( vector<sp<DB::AppSchema>> schemaNames, sp<QL::IQL> qlServer, UserPK executor )ε->void;
	}

	struct ResourcePermissions{ flat_map<ResourcePK,Resource> Resources; flat_map<PermissionPK,Permission> Permissions; };
	struct ResourceLoadAwait final : TAwait<ResourcePermissions>{
		ResourceLoadAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, UserPK executer )ι:
			_executer{executer}, _qlServer{qlServer}, _schemas{schemas}{};
	private:
		α Suspend()ι->void{ Load(); }
		α Load()ι->QL::QLAwait<jarray>::Task;

		UserPK _executer;
		sp<QL::IQL> _qlServer;
		vector<sp<DB::AppSchema>> _schemas;
	};
}