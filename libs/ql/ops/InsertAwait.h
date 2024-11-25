#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/InsertClause.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct Table; struct IDataSource; struct InsertClause; }
namespace Jde::QL{
	struct InsertAwait final: TAwait<jvalue>{
		InsertAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ InsertBefore(); }
		α await_resume()ε->jvalue override;
	private:
		α CreateQuery( const DB::Table& table, const jobject& input, bool nested=false )ε->void;
		α AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria={} )ε->void;
		α InsertBefore()ι->MutationAwaits::Task;
		α Execute()ι->Coroutine::Task;
		α InsertAfter( uint mainId )ι->MutationAwaits::Task;
		α InsertFailure()ι->MutationAwaits::Task;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		up<IException> _exception;
		vector<vector<sp<DB::Column>>> _missingColumns;
		flat_map<string,DB::Value> _nestedIds;
		vector<DB::InsertClause> _statements;
	};
}