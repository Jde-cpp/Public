#pragma once
#include <jde/db/sqlite_api.h>

struct sqlite3;

//Native twins of the server procs in the sibling mysql/sqlServer dirs - one <proc>.cpp per proc, same names,
//same param order, out params returned as the result row. RegisterProcs (jde/db/sqlite_api.h) registers them all
//through the driver's IProcs, so this DLL needn't link the driver.
namespace Jde::DB::Sqlite::AppProcs{
	α RegisterAppConnectionInsert( IProcs& procs )ι->void;
	α RegisterAppHostInsert( IProcs& procs )ι->void;
	α RegisterAppInstanceInsert( IProcs& procs )ι->void;
	α RegisterAppInstanceTagLevelUpsert( IProcs& procs )ι->void;
	α RegisterAppProgramInsert( IProcs& procs )ι->void;

	//Shared bodies - app_connection_insert finds-or-creates through these, like the sql procs `call` each other.
	α ProgramInsert( IProcs& procs, sqlite3& db, const Value& name, SL sl )ε->uint; //returns new program_id.
	α HostInsert( IProcs& procs, sqlite3& db, const Value& name, SL sl )ε->uint; //returns new host_id.
	α InstanceInsert( IProcs& procs, sqlite3& db, const Value& programId, const Value& name, const Value& hostName, SL sl )ε->uint; //returns new instance_id.
}
