#include "opcProcs.h"

//Twin of the *generated* opc_browse_name_insert proc (no mysql .sql file - SchemaDdl creates it from the table) -
//column order matches TableDdl::InsertProcCreateStatement: insertable columns in `i` order, sequence column out.
//	params: [0]=_ns, [1]=_name; out _browse_id returned as the result row.  Called by BrowseNameAwait::Create.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcBrowseNameInsert( IProcs& procs )ι->void{
		RegisterInsertProc( procs, "opc_browse_name_insert", "insert into opc_browse_names( ns, name ) values( ?, ? )", 2 );
	}
}
