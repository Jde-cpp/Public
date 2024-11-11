#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/InsertStatement.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct Table; struct IDataSource; struct InsertStatement; }
namespace Jde::QL{
	struct InsertAwait final: TAwait<jvalue>{
		InsertAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ Execute(); }
		α await_resume()ε->jvalue override;
	private:
		α CreateQuery( const DB::Table& table, const jobject& input, bool nested=false )ε->void;
		α AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria={} )ε->void;
		α Execute()ι->MutationAwaits::Task;
		α ExecuteProc()->Coroutine::Task;
		α InsertFailure()->MutationAwaits::Task;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		up<IException> _exception;
		vector<vector<sp<DB::Column>>> _missingColumns;
		flat_map<string,DB::Value> _nestedIds;
		vector<DB::InsertStatement> _statements;
	};
}