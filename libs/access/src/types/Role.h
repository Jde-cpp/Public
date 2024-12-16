#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include "../accessInternal.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	using PermissionRightPK=uint32;

	struct RoleRights final{
		Access::RolePK RolePK;
		Access::PermissionPK PermissionPK;
	};

	struct Role{
		Role( const jobject& j )ι;
		RolePK PK;
		bool Deleted;
		flat_set<PermissionRole> Members;
	};

	struct RoleLoadAwait final : TAwait<flat_map<RolePK,Role>>{
		RoleLoadAwait( sp<QL::IQL> qlServer, UserPK executer )ι:_executer{executer}, _qlServer{qlServer}{};
	private:
		α Suspend()ι->void override{ Load();}
		α Load()ι->QL::QLAwait::Task;
		UserPK _executer;
		sp<QL::IQL> _qlServer;
	};
}