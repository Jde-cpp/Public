#include "accessProcs.h"

#define let const auto

//Twin of the *generated* access_permission_insert proc - see TableDdl::InsertProcCreateStatement.
//	params: [0]=_is_role; out _permission_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessPermissionInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "access_permission_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let y = procs.ExecuteStatement( db, "insert into access_permissions( is_role ) values( ? )", {params[0]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{procs.LastInsertRowId(db)}} } ); //out _permission_id
			return y;
		}, 1);
	}
}
