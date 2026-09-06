#pragma once
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>

namespace Jde::DB::MySql{
	α ToRow( const mysql::row_view& row )ε->Row;
	//What a statement produced, as the Result every QueryAwait answers: a proc's OUT row first when one was asked for,
	//then the result set, and the affected count.  Empty when the statement produced no result at all (a DDL).
	α ToResult( const mysql::results& r, bool outParams )ε->Result;
}