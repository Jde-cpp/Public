#pragma once
#include "IAccessIdentity.h"
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/TaskOld.h>
#include "Group.h"

namespace Jde::DB{ struct Schema; }
namespace Jde::Access{
	struct AllowedDisallowed final{
		ERights Allowed;
		ERights Denied;
	};

	struct User final : IAccessIdentity{
		User( uint pk )ι:PK{pk}{}

		α Rights( sv resource )Ι->AllowedDisallowed;

		uint PK;
		flat_set<GroupPK> Groups;
	};

	struct UserLoadAwait final : TAwaitEx<concurrent_flat_map<UserPK,User>,Coroutine::Task>{
		UserLoadAwait( sp<DB::Schema> schema )ι;
	private:
		α Execute()ι->Coroutine::Task override;
		sp<DB::Schema> _schema;
	};
}