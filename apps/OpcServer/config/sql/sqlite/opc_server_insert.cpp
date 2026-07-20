#include "opcProcs.h"

#define let const auto

//Twin of the *generated* opc_server_insert proc (no mysql .sql file - SchemaDdl creates it from the table) - column
//order/defaults match TableDdl::InsertProcCreateStatement: insertable columns in `i` order, created=$now, sequence out.
//	params: [0]=_name, [1]=_target, [2]=_attributes, [3]=_description; out _server_id returned as the result row.
//	Called by StartupAwait when the server row is missing.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcServerInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "opc_server_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let y = procs.ExecuteStatement( db, "insert into opc_servers( name, target, attributes, created, description ) values( ?, ?, ?, unixepoch(), ? )", {params[0], params[1], params[2], params[3]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{procs.LastInsertRowId(db)}} } ); //out _server_id
			return y;
		});
	}
}
