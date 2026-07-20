#include "accessProcs.h"

#define let const auto

//Twin of the *generated* access_provider_insert proc - see TableDdl::InsertProcCreateStatement.  access_providers has
//no created/updated columns, so the insert is just the two insertable columns.
//	params: [0]=_provider_type_id, [1]=_target; out _provider_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessProviderInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "access_provider_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let y = procs.ExecuteStatement( db, "insert into access_providers( provider_type_id, target ) values( ?, ? )", {params[0], params[1]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{procs.LastInsertRowId(db)}} } ); //out _provider_id
			return y;
		}, 2);
	}
}
