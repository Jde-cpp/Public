#pragma once
#include "Sql.h"
#include "WhereClause.h"

namespace Jde::DB{
	struct Column;

	struct UpdateStatement final{
		α Move()ι->DB::Sql;
		α Add( sp<Column> column, Value value, SRCE )ε{ Values.try_emplace( column, move(value) ); }
		flat_map<sp<Column>,Value> Values;
		WhereClause Where;
	};
}