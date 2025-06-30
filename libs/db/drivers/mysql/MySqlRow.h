#pragma once
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>

namespace Jde::DB::MySql{
	α ToRow( const mysql::row_view& row, SRCE )ε->Row;
}