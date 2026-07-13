#pragma once
#include "exports.h"
#include <jde/db/sqlite_api.h> //ProcΛ, IProcs

struct sqlite3;

namespace Jde::DB::Sqlite{
	//Driver-internal registry. The app-facing surface is IProcs (in <jde/db/sqlite_api.h>) - proc DLLs register and
	//run statements through the IProcs the driver hands them, so they needn't link this driver.
	//The data source intercepts Sql::IsProc statements and dispatches to FindProc's result inside a transaction.
	auto RegisterProc( string name, ProcΛ proc )ι->void;
	auto UnregisterProcs( const vector<string>& names )ι->void; //remove a dll's procs before it unloads (see SqliteApi) so their ProcΛ std::functions aren't destroyed after dlclose.
	auto FindProc( sv name )ι->const ProcΛ*; //nullptr if not registered.
	auto RegisteredProcNames()ι->vector<string>; //for SqliteServerMeta::LoadProcs - DDL sync treats registered procs as existing.

	//Helpers for proc bodies - prepare/bind/step plain statements against `db`.
	auto ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint;
	auto ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint>;

	//The IProcs the driver hands to each proc DLL's RegisterProcs; forwards to the free functions above.
	//auto Procs()ι->IProcs&;
	//The IProcs handed to proc DLLs (see sqlite_api.h); forwards to the registry free functions above so a DLL
	//can register + run statements without linking this driver.
	struct ProcRegistry : IProcs{ //SqliteApi extends this to also track+unregister its dll's proc names.
		α RegisterProc( string name, ProcΛ proc )ι->void override{ Sqlite::RegisterProc( move(name), move(proc) ); }
		α ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint override{ return Sqlite::ExecuteStatement( db, sql, params, onRow, sl ); }
		α ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint> override{ return Sqlite::ScalarUInt( db, sql, params, sl ); }
	};
}
