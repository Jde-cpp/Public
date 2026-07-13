#pragma once
#include <jde/db/Row.h>
#include <jde/db/Value.h>
#include <jde/db/awaits/DBAwait.h> //RowΛ

struct sqlite3;

namespace Jde::DB::Sqlite{
	//A native twin of a server-side stored procedure (sqlite has no procs). Runs inside a transaction owned by the
	//caller; out params come back as a single result row, in declaration order. Returns rows affected.
	using ProcΛ = std::function<uint( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )>;

	//Registry + statement helpers the driver (Jde.DB.Sqlite) hands to each proc DLL's RegisterProcs. A DLL registers
	//its twins and runs statements through this, so it needn't link the driver - only <jde/db/sqlite_api.h>+Jde.DB.
	struct IProcs{
		β RegisterProc( string name, ProcΛ proc )ι->void =0;
		β ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint =0; //rows affected
		β ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint> =0;
	protected:
		virtual ~IProcs()=default;
	};
}

extern "C"{
	//Every proc DLL exports this single symbol. The driver dlopens the DLL and calls it with its registry when the
	//configured datasource is sqlite; the DLL registers all its native proc twins through `procs`.
	void RegisterProcs( Jde::DB::Sqlite::IProcs& procs );
}
