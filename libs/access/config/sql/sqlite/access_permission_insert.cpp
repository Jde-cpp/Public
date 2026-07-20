#include "accessProcs.h"

//Twin of the *generated* access_permission_insert proc - see TableDdl::InsertProcCreateStatement.
//	params: [0]=_is_role; out _permission_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessPermissionInsert( IProcs& procs )ι->void{
		RegisterInsertProc( procs, "access_permission_insert", "insert into access_permissions( is_role ) values( ? )", 1 );
	}
}
