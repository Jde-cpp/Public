#pragma once
#include <jde/db/usings.h>
#include <jde/db/Value.h>
#include <jde/framework/io/json.h>

namespace Jde::DB{ struct IDataSource; struct AppSchema; struct Statement; struct Syntax; struct WhereClause; }
namespace Jde::QL{
	struct TableQL;
	α Query( const TableQL& table, jobject& jData, UserPK userId )ε->void;
	α SelectStatement( sp<DB::AppSchema> schema, const TableQL& table, bool includeIdColumn/*, optional<DB::WhereClause> where=nullopt*/ )ι->optional<DB::Statement>;
}