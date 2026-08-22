#include "accessProcs.h"
#include "jde/db/generators/Sql.h"
#include <jde/db/DBException.h>

#define let const auto

//Twin of ../mysql/access_role_remove.sql.
//	params: [0]=_role_id, [1]=_permission_id; no out params.
//Both ids are client-supplied (RoleMAwait::RemovePermission binds them straight from the mutation), so the proc refuses a
//permission the role doesn't own and every delete is scoped to the role anyway: keyed on the permission alone, the first
//statement dropped the rights row of another role's permission - or of a direct acl grant - before the last one failed
//on its fk (access-review3 #2).  mysql/sqlServer autocommit per statement, so there the damage stuck.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRoleRemove( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_remove", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			constexpr sv isMember = "select 1 from access_role_members where role_id=? and member_id=?";
			THROW_IFX(
				!procs.ScalarUInt( db, isMember, {params[0], params[1]}, sl ),
				DBException( EDbError::App, Sql{string{isMember}, {params[0], params[1]}}, Ƒ("Permission '{}' is not a member of role '{}'.", params[1].ToString(), params[0].ToString()), {}, sl )
			);
			procs.ExecuteStatement( db, Ƒ("delete from access_permission_rights where permission_id=? and exists( {} )", isMember), {params[1], params[0], params[1]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_role_members where member_id=? and role_id=?", {params[1], params[0]}, nullptr, sl );
			return procs.ExecuteStatement( db,
				"delete from access_permissions where permission_id=?"
				" and not exists( select 1 from access_role_members where member_id=? )"
				" and not exists( select 1 from access_acl where permission_id=? )", {params[1], params[1], params[1]}, nullptr, sl );
		}, 2 );
	}
}
