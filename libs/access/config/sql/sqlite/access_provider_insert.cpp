#include "accessProcs.h"

//Twin of the *generated* access_provider_insert proc - see TableDdl::InsertProcCreateStatement.  access_providers has
//no created/updated columns, so the insert is just the two insertable columns.
//	params: [0]=_provider_type_id, [1]=_target; out _provider_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessProviderInsert( IProcs& procs )ι->void{
		RegisterInsertProc( procs, "access_provider_insert", "insert into access_providers( provider_type_id, target ) values( ?, ? )", 2 );
	}
}
