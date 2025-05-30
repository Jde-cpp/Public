#pragma once
#include <jde/access/usings.h>
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Permission.h>

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{
	struct AclLoadAwait final : TAwait<flat_multimap<IdentityPK,PermissionRole>>{
		AclLoadAwait( sp<QL::IQL> qlServer, UserPK executer )ι:_executer{executer}, _qlServer{qlServer}{};
	private:
		α Suspend()ι->void override{ Load(); }
		α Load()ι->QL::QLAwait<jarray>::Task;
		UserPK _executer;
		sp<QL::IQL> _qlServer;
	};
}