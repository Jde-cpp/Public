#include "appProcs.h"

#define let const auto

//Twin of the *generated* app_host_insert proc - column order/defaults match TableDdl::InsertProcCreateStatement for
//app_hosts: the insertable columns in `i` order, the sequence column returned as the out row.
//	params: [0]=_name; out _host_id returned as the result row.
namespace Jde::DB::Sqlite::AppProcs{
	α HostInsert( IProcs& procs, sqlite3& db, const Value& name, SL sl )ε->uint{
		procs.ExecuteStatement( db, "insert into app_hosts( name ) values( ? )", {name}, nullptr, sl );
		return procs.LastInsertRowId( db );
	}

	α RegisterAppHostInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "app_host_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let hostId = HostInsert( procs, db, params[0], sl );
			if( onRow )
				(*onRow)( Row{ {Value{hostId}} } ); //out _host_id
			return 1; //one row - see ProcΛ.
		}, 1);
	}
}
