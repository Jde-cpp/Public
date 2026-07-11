#include <sqlite3.h>
#include "opcProcs.h"

//Twin of ../mysql/opc_node_insert.sql.
//	params: [0]=_node_id, [1]=_parent_node_id, [2]=_ref_id, [3]=_type_id, [4]=_obj_attr_id,
//	[5]=_object_type_attr_id, [6]=_variable_attr_id, [7]=_browse_id; no out params.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcNodeInsert()ι->void{
		RegisterProc( "opc_node_insert", []( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			return ExecuteStatement( db,
				"insert into opc_nodes( node_id, parent_node_id, reference_type_id, type_definition_id, object_attr_id, object_type_attr_id, variable_attr_id, browse_id )"
				" values( ?, ?, ?, ?, ?, ?, ?, ? )", {params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]}, nullptr, sl );
		});
	}
}
