#pragma once
#include <jde/access/usings.h>
//#include "../types/User.h"
//#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>


namespace Jde::QL{ struct IQL; }
namespace Jde::Access{
	struct Group; struct User;
	struct Identities{
		flat_map<UserPK,User> Users;
		flat_map<GroupPK,Group> Groups;
	};

	struct IdentityLoadAwait final : TAwait<Identities>{
		IdentityLoadAwait( sp<QL::IQL> ql, UserPK executer, SRCE )ι:TAwait<Identities>{sl},_executer{executer},_ql{ql}{};
		α Suspend()ι->void override{ Load(); }
	private:
		α Load()ι->QL::QLAwait::Task;
		UserPK _executer;
		sp<QL::IQL> _ql;
	};
}