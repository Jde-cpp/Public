#pragma once
#include "exports.h"
#include <jde/db/Row.h>
#include <jde/db/Value.h>
#include <jde/db/awaits/DBAwait.h> //RowΛ

struct sqlite3;

namespace Jde::DB::Sqlite{
	//Native replacement for a server-side stored procedure. sqlite has no procs, so each `<name>.sql` proc from
	//config/sql/{sqlServer,mysql} gets a C++ twin registered under the same name. The data source intercepts
	//Sql::IsProc statements and dispatches here - callers are unaware they aren't hitting a server proc.
	//Runs inside a transaction owned by the caller (SqliteDataSource::Execute). Out params are returned as a
	//single result row (same shape MySql's out_params() produces), in declaration order.
	//Returns rows affected.
	using ProcΛ = std::function<uint( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )>;

	ΓLITE auto RegisterProc( string name, ProcΛ proc )ι->void;
	ΓLITE auto FindProc( sv name )ι->const ProcΛ*; //nullptr if not registered.
	ΓLITE auto RegisteredProcNames()ι->vector<string>; //for SqliteServerMeta::LoadProcs - DDL sync treats registered procs as existing.

	//Helpers for proc bodies - prepare/bind/step plain statements against `db`.
	ΓLITE auto ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint;
	ΓLITE auto ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint>;
}
