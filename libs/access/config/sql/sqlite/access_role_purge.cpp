#include "accessProcs.h"

//Twin of ../mysql/access_role_purge.sql.
//	params: [0]=_role_id; no out params.
//The role's inbound references go first - the acl rows granting it (or one of its permissions) to an identity, and its own
//membership in a parent role - or the final delete of its access_permissions row fails on their fk, which on the autocommit
//dialects left the role half-destroyed (access-review3 #13).
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRolePurge( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_purge", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			procs.ExecuteStatement( db, "delete from access_acl where permission_id=? or permission_id in ( select member_id from access_role_members where role_id=? )", {params[0], params[0]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_role_members where member_id=?", {params[0]}, nullptr, sl );
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
