#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/framework/coroutine/Await.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/ql/QLHook.h>

namespace Jde::DB{ struct Table; struct IDataSource; }
namespace Jde::QL{
	struct ChildParentParams final{ sp<DB::Column> ParentCol; sp<DB::Column> ChildColumn; DB::Value ParentParam; vector<DB::Value> ChildParams; };
	struct AddRemoveAwait final: TAwait<jvalue>{
		AddRemoveAwait( sp<DB::Table> table, const MutationQL& mutation, UserPK userPK, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α AddHook()ι->MutationAwaits::Task;
		α RemoveHook()ι->MutationAwaits::Task;
		α Add()ι->Coroutine::Task;
		α Remove()->Coroutine::Task;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		up<IException> _exception;
		ChildParentParams _params;
	};
}