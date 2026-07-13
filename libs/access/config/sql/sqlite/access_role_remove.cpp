#include <sqlite3.h>
#include "accessProcs.h"

//Twin of ../mysql/access_role_remove.sql.
//	params: [0]=_role_id, [1]=_permission_id; no out params.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRoleRemove( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_remove", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			procs.ExecuteStatement( db, "delete from access_permission_rights where permission_id=?", {params[1]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_role_members where member_id=? and role_id=?", {params[1], params[0]}, nullptr, sl );
			return procs.ExecuteStatement( db, "delete from access_permissions where permission_id=?", {params[1]}, nullptr, sl );
		});
	}
}
