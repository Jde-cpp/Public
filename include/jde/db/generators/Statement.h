#pragma once
#include <jde/db/Value.h>
#include <jde/db/generators/FromClause.h>

namespace Jde::DB{
	struct Column; struct FromClause; struct JoinClause; struct Table; struct WhereClause;
	struct Statement final{
//		Statement( string&& sql, vector<DB::Value>&& params )ι:Sql{move(sql)}, Parameters{params} {}
		string Sql;
		vector<Value> Params;
	};
//namespace Generate{
	//α Generate( const vector<string>& columns, const vector<string>& tables )ε->string;
	α SelectSql( vec<sp<Column>> columns, FromClause from, WhereClause where, SRCE )ε->Statement;
	α SelectSql( vec<string> columns, FromClause from, WhereClause where, SRCE )ε->Statement;
	α SelectSKsSql( sp<Table> table )ε->Statement;
}