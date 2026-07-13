#pragma once
#include <jde/db/Row.h>
#include <jde/db/Value.h>
#include <jde/db/awaits/DBAwait.h> //RowΛ

struct sqlite3_stmt;

namespace Jde::DB::Sqlite{
	//Bind DB::Value params to a prepared statement, 1-based per sqlite convention.
	α Bind( sqlite3_stmt& stmt, const vector<Value>& params, SL sl )ε->void;
	//Convert the current SQLITE_ROW into a DB::Row.
	α ToRow( sqlite3_stmt& stmt )ε->Row;
	α ToValue( sqlite3_stmt& stmt, int col )ε->Value;
}
