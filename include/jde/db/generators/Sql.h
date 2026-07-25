#pragma once
#include <jde/db/Value.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/exports.h>

#define Φ ΓDB α
namespace Jde::DB{
	struct Column; struct FromClause; struct JoinClause; struct Table; struct WhereClause;
	struct ΓDB Sql final{ //the drivers are separate modules: operator+= and EmbedParams are out-of-line in Jde.DB.
		α operator+=( WhereClause&& sql )ι->Sql&;
		string Text;
		vector<Value> Params;
		bool IsProc{ false };
		α EmbedParams()Ι->string;
	};

	Φ SelectSql( vec<sp<Column>> columns, FromClause from, WhereClause where, SRCE )ε->Sql;
	Φ SelectSql( vec<string> columns, FromClause from, WhereClause where, SRCE )ε->Sql;
	Φ SelectSKsSql( sp<Table> table )ε->Sql;
}
#undef Φ