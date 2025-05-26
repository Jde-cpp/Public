#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/meta/Column.h>
#include <jde/db/generators/InsertClause.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct Criteria; struct IDataSource; struct InsertClause; struct Table; }
namespace Jde::QL{
	struct InsertAwait final: TAwait<jvalue>{
		using base=TAwait<jvalue>;
		InsertAwait( sp<DB::Table> table, MutationQL mutation, UserPK executer, SRCE )ι;
		InsertAwait( sp<DB::Table> table, MutationQL&& m, bool identityInsert, UserPK executer, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ InsertBefore(); }
		α await_resume()ε->jvalue override;
	private:
		α CreateQuery( const DB::Table& table, const jobject& input, bool nested=false )ε->void;
		α AddStatement( const DB::Table& table, const jobject& input, optional<DB::Criteria> criteria=nullopt )ε->void;
		α InsertBefore()ι->MutationAwaits::Task;
		α Execute()ι->Coroutine::Task;
		α InsertAfter( jarray&& result )ι->MutationAwaits::Task;
		α InsertFailure( exception e )ι->MutationAwaits::Task;
		α Resume( jarray&& v )ι->void;

		UserPK _executer;
		bool _identityInsert;
		const MutationQL _mutation;
		sp<DB::Table> _table;

		up<IException> _exception;
		vector<vector<sp<DB::Column>>> _missingColumns;
		flat_map<string,DB::Value> _nestedIds;
		vector<DB::InsertClause> _statements;
	};
}