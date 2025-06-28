#pragma once
#include "Sql.h"
#include "WhereClause.h"

namespace Jde::DB{
	struct Column;

	struct ΓDB UpdateClause final{
		α Move()ι->DB::Sql;
		α Add( sp<Column> column, Value value )ε->void;
		flat_map<sp<Column>,Value> Values;
		WhereClause Where;
	};
}