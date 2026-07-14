#include "accessProcs.h"

//Twin of ../mysql/access_role_purge.sql.
//	params: [0]=_role_id; no out params.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRolePurge( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_purge", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			procs.ExecuteStatement( db, "delete from access_permission_rights where permission_id in ( select member_id from access_role_members where role_id=? )", {params[0]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_role_members where role_id=?", {params[0]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_roles where role_id=?", {params[0]}, nullptr, sl );
			return procs.ExecuteStatement( db,
				"delete from access_permissions"
				" where permission_id not in ( select member_id from access_role_members where role_id=? )"
				" and permission_id not in ( select permission_id from access_permission_rights )"
				" and permission_id not in ( select role_id from access_roles )", {params[0]}, nullptr, sl );
		});
	}
}
