#pragma once
#include <jde/db/sqlite_api.h>

struct sqlite3;

//Native twin of the server proc in the sibling mysql dir - same name, same param order, out param returned
//as the result row. RegisterProcs (jde/db/sqlite_api.h) registers it through the driver's IProcs.
namespace Jde::DB::Sqlite::GatewayProcs{
	α RegisterGatewayServerConnectionInsert( IProcs& procs )ι->void;
}
