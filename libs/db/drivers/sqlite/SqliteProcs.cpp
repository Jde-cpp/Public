#include <sqlite3.h>
#include "SqliteProcs.h"
#include "SqliteRow.h" //Bind/ToRow
#include <jde/db/DBException.h>

#define let const auto

namespace Jde::DB::Sqlite{
	flat_map<string,ProcΛ> _procs; std::shared_mutex _procsMutex;

	α RegisterProc( string name, ProcΛ proc )ι->void{
		std::unique_lock l{ _procsMutex };
		_procs[move(name)] = move(proc);
	}
	α FindProc( sv name )ι->const ProcΛ*{
		std::shared_lock l{ _procsMutex };
		let p = _procs.find( string{name} );
		return p==_procs.end() ? nullptr : &p->second;
	}

	α ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint{
		sqlite3_stmt* stmt{};
		THROW_IFSL( sqlite3_prepare_v2(&db, sql.data(), (int)sql.size(), &stmt, nullptr)!=SQLITE_OK, "prepare failed: {} - {}", sqlite3_errmsg(&db), sql );
		std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> cleanup{ stmt, &sqlite3_finalize };
		Bind( *stmt, params, sl );
		int rc;
		while( (rc=sqlite3_step(stmt))==SQLITE_ROW ){
			if( onRow )
				(*onRow)( ToRow(*stmt) );
		}
		THROW_IFSL( rc!=SQLITE_DONE, "step failed: {} - {}", sqlite3_errmsg(&db), sql );
		return (uint)sqlite3_changes( &db );
	}

	α ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint>{
		optional<uint> y;
		RowΛ f = [&y]( Row&& row ){ if( !row.IsNull(0) ) y = row.GetUInt(0); };
		ExecuteStatement( db, sql, params, &f, sl );
		return y;
	}

	//Example twin of apps/AppServer/config/sql/{mysql,sqlServer}/app_instance_insert.sql.
	//Registration belongs in the owning app (e.g. AppServer startup), not here - shown for the pattern.
	//	params: [0]=_programId, [1]=_name, [2]=_hostName; out _instanceId returned as the single result row.
	α RegisterAppServerProcs()ι->void{
		RegisterProc( "app_instance_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			auto hostId = ScalarUInt( db, "select host_id from app_hosts where name=?", {params[2]}, sl );
			if( !hostId ){
				ExecuteStatement( db, "insert into app_hosts( name ) values( ? )", {params[2]}, nullptr, sl );
				hostId = (uint)sqlite3_last_insert_rowid( &db );
			}
			let y = ExecuteStatement( db, "insert into app_instances( program_id, name, host_id ) values( ?, ?, ? )", {params[0], params[1], Value{*hostId}}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{(uint)sqlite3_last_insert_rowid(&db)}} } ); //out _instanceId
			return y;
		});
	}
}
