#pragma once
#include "../../../../../libs/db/drivers/sqlite/src/SqliteProcs.h"

struct sqlite3;

//Native twin of the server proc in the sibling mysql dir - same name, same param order, out param returned
//as the result row. RegisterAppServerProcs (jde/db/sqlite_api.h) registers it.
namespace Jde::DB::Sqlite::GatewayProcs{
	α RegisterGatewayServerConnectionInsert()ι->void;
}
