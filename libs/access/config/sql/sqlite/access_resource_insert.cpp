#include "accessProcs.h"

#define let const auto

//Twin of the *generated* access_resource_insert proc - column order/defaults match TableDdl::InsertProcCreateStatement
//for access_resources: insertable columns in `i` order, created=$now, the sequence column out last.
//	params: [0]=_schema_name, [1]=_name, [2]=_target, [3]=_attributes, [4]=_description, [5]=_criteria, [6]=_allowed;
//	out _resource_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessResourceInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "access_resource_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let y = procs.ExecuteStatement( db, "insert into access_resources( schema_name, name, target, attributes, created, description, criteria, allowed ) values( ?, ?, ?, ?, unixepoch(), ?, ?, ? )", {params[0], params[1], params[2], params[3], params[4], params[5], params[6]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{procs.LastInsertRowId(db)}} } ); //out _resource_id
			return y;
		});
	}
}
