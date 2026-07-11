#include <sqlite3.h>
#include "opcProcs.h"

//Twin of ../mysql/opc_constructor_insert.sql.
//	params: [0]=_node_id, [1]=_browse_id, [2]=_variant_id; no out params.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcConstructorInsert()ι->void{
		RegisterProc( "opc_constructor_insert", []( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			return ExecuteStatement( db, "insert into opc_constructors( node_id, browse_id, variant_id ) values( ?, ?, ? )", {params[0], params[1], params[2]}, nullptr, sl );
		});
	}
}
