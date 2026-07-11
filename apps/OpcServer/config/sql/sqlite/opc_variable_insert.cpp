#include <sqlite3.h>
#include "opcProcs.h"
#include <jde/db/DBException.h>

#define let const auto

//Twin of ../mysql/opc_variable_insert.sql - node_id stays null when no id part is supplied, and the out param
//is returned null in that case, matching the mysql proc.
//	params: [0]=_ns, [1]=_number, [2]=_string, [3]=_guid, [4]=_bytes, [5]=_parent_node_id, [6]=_ref_type_id,
//	[7]=_type_id, [8]=_browse_id, [9]=_specified, [10]=_name, [11]=_description, [12]=_write_mask,
//	[13]=_user_write_mask, [14]=_variant_id, [15]=_data_type_id, [16]=_value_rank, [17]=_array_dims,
//	[18]=_access_level, [19]=_user_access_level, [20]=_minimum_sampling_interval, [21]=_historizing;
//	out _node_id returned as the result row.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcVariableInsert()ι->void{
		RegisterProc( "opc_variable_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			THROW_IFSL( !params[15].is_null() && params[15].get_number<uint>(sl)==0, "Data type ID cannot be zero" );
			Value nodeId;
			if( !params[1].is_null() || !params[2].is_null() || !params[3].is_null() || !params[4].is_null() )
				nodeId = Value{ NodeIdInsert(db, params[0], params[1], params[2], params[3], params[4], Value{}, Value{}, Value{}, sl) };
			EnsureDataTypeNodeId( db, params[15], sl );
			let y = ExecuteStatement( db,
				"insert into opc_variables( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name, description, write_mask, user_write_mask,"
				" variant_id, data_type_id, value_rank, array_dims, access_level, user_access_level, minimum_sampling_interval, historizing )"
				" values( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )",
				{nodeId, params[5], params[6], params[7], params[8], params[9], params[10], params[11], params[12], params[13], params[14], params[15], params[16], params[17], params[18], params[19], params[20], params[21]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {nodeId} } ); //out _node_id - null when not a node.
			return y;
		});
	}
}
