#pragma once
#include <jde/ql/GraphQLHook.h>
#include <jde/ql/types/MutationQL.h>
#include "../GraphQL.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/db/meta/Table.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::QL{
	struct PurgeAwait final: TAwait<jvalue>{
		PurgeAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α Suspend()ι->void override{ Before(); }
	private:
		α Before()ι->MutationAwaits::Task;
		α Statements( const DB::Table& table, vector<DB::Value>& parameters )->vector<string>;
		α Execute()ι->Coroutine::Task;
		α After( up<IException>&& e )ι->MutationAwaits::Task;
		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
	};
}