#pragma once
#include "../../../../../libs/db/drivers/sqlite/src/SqliteProcs.h"

struct sqlite3;

//Native twins of the server procs in the sibling mysql/sqlServer dirs - one <proc>.cpp per proc, same names,
//same param order, out params returned as the result row. RegisterAppServerProcs (jde/db/sqlite_api.h) registers them all.
namespace Jde::DB::Sqlite::AppProcs{
	α RegisterAppConnectionInsert()ι->void;
	α RegisterAppInstanceInsert()ι->void;
	α RegisterAppInstanceTagLevelUpsert()ι->void;
	α RegisterAppProgramInsert()ι->void;

	//Shared bodies - app_connection_insert finds-or-creates through these, like the sql procs `call` each other.
	α ProgramInsert( sqlite3& db, const Value& name, SL sl )ε->uint; //returns new program_id.
	α InstanceInsert( sqlite3& db, const Value& programId, const Value& name, const Value& hostName, SL sl )ε->uint; //returns new instance_id.
}
