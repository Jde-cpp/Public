#include <sqlite3.h>
#include "appProcs.h"

#define let const auto

//Twin of ../mysql/app_instance_insert.sql - finds-or-creates the host.
//	params: [0]=_programId, [1]=_name, [2]=_hostName; out _instanceId returned as the result row.
namespace Jde::DB::Sqlite::AppProcs{
	α InstanceInsert( sqlite3& db, const Value& programId, const Value& name, const Value& hostName, SL sl )ε->uint{
		auto hostId = ScalarUInt( db, "select host_id from app_hosts where name=?", {hostName}, sl );
		if( !hostId ){
			ExecuteStatement( db, "insert into app_hosts( name ) values( ? )", {hostName}, nullptr, sl );
			hostId = (uint)sqlite3_last_insert_rowid( &db );
		}
		ExecuteStatement( db, "insert into app_instances( program_id, name, host_id ) values( ?, ?, ? )", {programId, name, Value{*hostId}}, nullptr, sl );
		return (uint)sqlite3_last_insert_rowid( &db );
	}

	α RegisterAppInstanceInsert()ι->void{
		RegisterProc( "app_instance_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let instanceId = InstanceInsert( db, params[0], params[1], params[2], sl );
			if( onRow )
				(*onRow)( Row{ {Value{instanceId}} } ); //out _instanceId
			return 1;
		});
	}
}
