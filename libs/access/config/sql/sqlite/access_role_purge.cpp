#include "accessProcs.h"

#define let const auto

//Twin of ../mysql/access_role_purge.sql.
//	params: [0]=_role_id; no out params.
//The role's inbound references go first - the acl rows granting it and its own membership in a parent role - or the final
//delete of its access_permissions row fails on their fk, which on the autocommit dialects left the role half-destroyed
//(access-review3 #13).
//The role's own permissions - the is_role=false members access_role_add minted for it - go with it.  A member that is
//itself a role is shared and keeps its acl grants, rights and other memberships:  only this role's membership row goes
//(the old `permission_id in ( members )` deletes stripped every direct grant of every child role).  The same courtesy
//goes to a bare member something else still references - another role's membership, an acl row.  The owned members are
//collected first:  the membership rows must go before the permission rows can (fk), and would take the list with them.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRolePurge( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_purge", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			vector<Value> owned;
			RowΛ collect = [&owned]( Row&& row ){ owned.push_back( Value{row.Get<uint>(0)} ); };
			procs.ExecuteStatement( db,
				"select m.member_id from access_role_members m join access_permissions p on p.permission_id=m.member_id"
				" where m.role_id=? and p.is_role=0"
				" and not exists( select 1 from access_role_members o where o.member_id=m.member_id and o.role_id<>? )"
				" and not exists( select 1 from access_acl a where a.permission_id=m.member_id )", {params[0], params[0]}, &collect, sl );

			procs.ExecuteStatement( db, "delete from access_acl where permission_id=?", {params[0]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_role_members where role_id=? or member_id=?", {params[0], params[0]}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_permission_rights where permission_id=?", {params[0]}, nullptr, sl );
			for( let& permissionId : owned )
				procs.ExecuteStatement( db, "delete from access_permission_rights where permission_id=?", {permissionId}, nullptr, sl );
			procs.ExecuteStatement( db, "delete from access_roles where role_id=?", {params[0]}, nullptr, sl );
			for( let& permissionId : owned )
				procs.ExecuteStatement( db, "delete from access_permissions where permission_id=?", {permissionId}, nullptr, sl );
			return procs.ExecuteStatement( db, "delete from access_permissions where permission_id=?", {params[0]}, nullptr, sl );
		}, 1 );
	}
}
