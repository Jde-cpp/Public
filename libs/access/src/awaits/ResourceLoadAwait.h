#pragma once
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Permission.h>
#include <jde/access/types/Resource.h>

namespace Jde::DB{ struct IRow; struct AppSchema; }
namespace Jde::QL{ struct IQL; }
namespace Jde::Access{
	struct ResourceSyncAwait final : VoidAwait, boost::noncopyable{
		ResourceSyncAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, UserPK executer )ι:
			_executer{executer}, _qlServer{qlServer}, _schemas{schemas}{};
	private:
		α Suspend()ι->void{ Sync(); }
		α Sync()ι->TAwait<jvalue>::Task;
		UserPK _executer;
		sp<QL::IQL> _qlServer;
		vector<sp<DB::AppSchema>> _schemas;
	};

	struct ResourcePermissions{ flat_map<ResourcePK,Resource> Resources; flat_map<PermissionPK,Permission> Permissions; };
	struct ResourceLoadAwait final : TAwait<ResourcePermissions>, boost::noncopyable{
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