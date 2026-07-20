#include "opcProcs.h"

#define let const auto

//Twin of the *generated* opc_browse_name_insert proc (no mysql .sql file - SchemaDdl creates it from the table) -
//column order matches TableDdl::InsertProcCreateStatement: insertable columns in `i` order, sequence column out.
//	params: [0]=_ns, [1]=_name; out _browse_id returned as the result row.  Called by BrowseNameAwait::Create.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcBrowseNameInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "opc_browse_name_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let y = procs.ExecuteStatement( db, "insert into opc_browse_names( ns, name ) values( ?, ? )", {params[0], params[1]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{procs.LastInsertRowId(db)}} } ); //out _browse_id
			return y;
		}, 2);
	}
}
