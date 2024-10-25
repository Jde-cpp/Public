#pragma once
#include "Sql.h"
#include "WhereClause.h"

namespace Jde::DB{
	struct Column; struct Value;
	struct UpdateStatement{

		α Move()ι->DB::Sql;
		α Add( sp<Column> column, Value value, SRCE )ε{ Values.emplace_back( column, move(value) ); }
		vector<tuple<sp<Column>,Value>> Values;
		WhereClause Where;
	};
}