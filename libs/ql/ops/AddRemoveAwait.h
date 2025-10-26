#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/fwk/co/Await.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/ql/QLHook.h>

namespace Jde::DB{ struct Table; struct IDataSource; }
namespace Jde::QL{
	struct ChildParentParams final{ sp<DB::Column> ParentCol; sp<DB::Column> ChildColumn; DB::Value ParentParam; vector<DB::Value> ChildParams; };
	struct AddRemoveAwait final: TAwait<jvalue>{
		using base=TAwait<jvalue>;
		AddRemoveAwait( sp<DB::Table> table, const MutationQL& mutation, UserPK userPK, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α AddBefore()ι->MutationAwaits::Task;
		α AddHook()ι->MutationAwaits::Task;
		α Add()ι->DB::ExecuteAwait::Task;
		α AddAfter( jvalue v )ι->MutationAwaits::Task;
		α RemoveHook()ι->MutationAwaits::Task;
		α Remove()->DB::ExecuteAwait::Task;
		α RemoveAfter( jvalue v )ι->MutationAwaits::Task;
		α Resume( jvalue&& v )ι->void;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		//up<IException> _exception;
		ChildParentParams _params;
		jobject _variables;
	};
}