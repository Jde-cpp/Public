#pragma once
#include "../exports.h"
#include "Sql.h"

namespace Jde::DB{
	struct Column;
	struct ΓDB InsertClause final{
		InsertClause()ι=default;
		InsertClause( sv name )ι:_procName{ name }{}
		InsertClause( sv name, vector<Value>&& params )ι;
		α Move()ι->DB::Sql;
		α Add( sp<Column> column, Value::Underlying value )ι->void;
		α Add( Value::Underlying value )ι->void;
		//Ŧ Add( PK<T> pk )ι{ Values.emplace_back( make_pair(sp<Column>{}, pk.Value) ); }
		α SetValue( sp<Column> column, Value value )ε->void;
		α SequenceColumn()Ι->sp<Column>;
		vector<std::pair<sp<Column>,Value>> Values;
		bool IsStoredProc{ true };
	private:
		α Proc( str procName )ι->DB::Sql;
		α Insert( str tableName )ι->DB::Sql;

		optional<string> _procName;
	};
}