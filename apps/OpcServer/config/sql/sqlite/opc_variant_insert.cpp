#include <sqlite3.h>
#include "opcProcs.h"

#define let const auto

//Twin of ../mysql/opc_variant_insert.sql.
//	params: [0]=_data_type_id, [1]=_array_dims; out _variant_id returned as the result row.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcVariantInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "opc_variant_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			EnsureDataTypeNodeId( procs, db, params[0], sl );
			let y = procs.ExecuteStatement( db, "insert into opc_variants( data_type_id, array_dims ) values( ?, ? )", {params[0], params[1]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{(uint)sqlite3_last_insert_rowid(&db)}} } ); //out _variant_id
			return y;
		});
	}
}
