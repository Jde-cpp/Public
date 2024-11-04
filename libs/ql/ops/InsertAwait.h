#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/db/generators/InsertStatement.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct Table; struct IDataSource; struct InsertStatement; }
namespace Jde::QL{
	struct InsertAwait final: AsyncReadyAwait{
		InsertAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK, SRCE )ι;
		α CreateQuery( const DB::Table& table )ι->optional<AwaitResult>;
		α Execute( HCoroutine h, UserPK userPK, sp<DB::IDataSource> ds )ι->Task;
	private:
		α CreateQuery( const DB::Table& table, const jobject& input, bool nested=false )ε->void;
		α AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria={} )ε->void;
		const MutationQL _mutation;
		vector<DB::InsertStatement> _statements;
		vector<vector<sp<DB::Column>>> _missingColumns;
		flat_map<string,DB::Value> _nestedIds;
	};
}