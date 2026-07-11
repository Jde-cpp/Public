#include <sqlite3.h>
#include "opcProcs.h"

#define let const auto

//Twin of ../mysql/opc_object_type_insert.sql - the opc_node_id_insert `call` (is_global=true) goes through
//NodeIdInsert. Like the mysql body, _specified is accepted but not inserted.
//	params: [0]=_ns, [1]=_number, [2]=_string, [3]=_guid, [4]=_bytes, [5]=_parent_node_id, [6]=_ref_type_id,
//	[7]=_browse_id, [8]=_specified, [9]=_name, [10]=_description, [11]=_write_mask, [12]=_user_write_mask,
//	[13]=_is_abstract; out _node_id returned as the result row.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcObjectTypeInsert()ι->void{
		RegisterProc( "opc_object_type_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let nodeId = NodeIdInsert( db, params[0], params[1], params[2], params[3], params[4], Value{}, Value{}, Value{true}, sl );
			let y = ExecuteStatement( db,
				"insert into opc_object_types( node_id, parent_node_id, ref_type_id, browse_id, name, description, write_mask, user_write_mask, is_abstract )"
				" values( ?, ?, ?, ?, ?, ?, ?, ?, ? )", {Value{nodeId}, params[5], params[6], params[7], params[9], params[10], params[11], params[12], params[13]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{nodeId}} } ); //out _node_id
			return y;
		});
	}
}
