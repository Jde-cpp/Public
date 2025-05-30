#pragma once
#include <jde/db/Value.h>
#include <jde/db/generators/FromClause.h>

namespace Jde::DB{
	struct Column; struct FromClause; struct JoinClause; struct Table; struct WhereClause;
	struct Sql final{

		α operator+=( WhereClause&& sql )ι->Sql&;
		string Text;
		vector<Value> Params;
	};

	α SelectSql( vec<sp<Column>> columns, FromClause from, WhereClause where, SRCE )ε->Sql;
	α SelectSql( vec<string> columns, FromClause from, WhereClause where, SRCE )ε->Sql;
	α SelectSKsSql( sp<Table> table )ε->Sql;
}