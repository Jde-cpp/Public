#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/access/types/Role.h>

namespace Jde::Access{
	struct RoleLoadAwait final : TAwait<flat_map<RolePK,Role>>{
		RoleLoadAwait( sp<QL::IQL> qlServer, UserPK executer )ι:_executer{executer}, _qlServer{qlServer}{};
	private:
		α Suspend()ι->void override{ Load();}
		α Load()ι->QL::QLAwait::Task;
		UserPK _executer;
		sp<QL::IQL> _qlServer;
	};
}