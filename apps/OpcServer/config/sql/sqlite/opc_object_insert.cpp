#include <sqlite3.h>
#include "opcProcs.h"

#define let const auto

//Twin of ../mysql/opc_object_insert.sql - the opc_node_id_insert `call` (is_global=true) goes through NodeIdInsert.
//	params: [0]=_ns, [1]=_number, [2]=_string, [3]=_guid, [4]=_bytes, [5]=_parent_node_id, [6]=_ref_type_id,
//	[7]=_type_def_id, [8]=_browse_id, [9]=_specified, [10]=_name, [11]=_description, [12]=_write_mask,
//	[13]=_user_write_mask, [14]=_event_notifier; out _node_id returned as the result row.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcObjectInsert()ι->void{
		RegisterProc( "opc_object_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let nodeId = NodeIdInsert( db, params[0], params[1], params[2], params[3], params[4], Value{}, Value{}, Value{true}, sl );
			let y = ExecuteStatement( db,
				"insert into opc_objects( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name, description, write_mask, user_write_mask, event_notifier )"
				" values( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )", {Value{nodeId}, params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12], params[13], params[14]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{nodeId}} } ); //out _node_id
			return y;
		});
	}
}
