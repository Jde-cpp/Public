#include "opcProcs.h"

//Twin of the *generated* opc_server_insert proc (no mysql .sql file - SchemaDdl creates it from the table) - column
//order/defaults match TableDdl::InsertProcCreateStatement: insertable columns in `i` order, created=$now, sequence out.
//	params: [0]=_name, [1]=_target, [2]=_attributes, [3]=_description; out _server_id returned as the result row.
//	Called by StartupAwait when the server row is missing.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcServerInsert( IProcs& procs )ι->void{
		RegisterInsertProc( procs, "opc_server_insert", "insert into opc_servers( name, target, attributes, created, description ) values( ?, ?, ?, unixepoch(), ? )", 4 );
	}
}
