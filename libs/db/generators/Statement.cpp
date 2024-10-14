#include <jde/db/generators/Statement.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>

#define let const auto

namespace Jde{
	α DB::SelectSql( const vector<sp<Column>>& columns, FromClause from, WhereClause where, SL sl )ε->Statement{
		string sql; sql.reserve( 1023 );
		from.SetActive( where, sl );

		sql += "select ";
		for( let& column : columns )
			sql += column->Table->DBName+'.'+column->Name + ',';
		sql[sql.size()-1] = '\n';
		sql += "from ";
		sql += from.ToString();
		sql += where.Move();
		return {move(sql), move(where.Params())};
	}

	α DB::SelectSql( vec<string> columns, FromClause from, WhereClause where, SL sl )ε->Statement{
		vector<sp<Column>> cols;
		for( let& column : columns )
			cols.push_back( from.GetColumnPtr(column, sl) );
		return SelectSql( cols, move(from), where );
	}

	α DB::SelectSKsSql( sp<Table> table )ε->Statement{
		return SelectSql( table->SurrogateKeys, {table}, {} );
	}

}