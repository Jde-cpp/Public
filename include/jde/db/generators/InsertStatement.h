#pragma once
#include "Sql.h"

namespace Jde::DB{
	struct Column;
	struct InsertStatement final{
		α Move()ι->DB::Sql;
		α Add( sp<Column> column, Value value )ι{ Values.emplace_back( make_pair(column, move(value)) ); }
		α SetValue( sp<Column> column, Value value )ε->void;
		vector<std::pair<sp<Column>,Value>> Values;
		bool IsStoredProc{ true };
	private:
		α Proc( str procName )ι->DB::Sql;
		α Insert( str tableName )ι->DB::Sql;
	};
}