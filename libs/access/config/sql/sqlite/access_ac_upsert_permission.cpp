#include "accessProcs.h"

#define let const auto

//Twin of ../mysql/access_ac_upsert_permission.sql.
//	params: [0]=_identityId, [1]=_allowed, [2]=_denied, [3]=_resourceId; out _permission_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessAcUpsertPermission( IProcs& procs )ι->void{
		procs.RegisterProc( "access_ac_upsert_permission", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			auto permissionId = procs.ScalarUInt( db,
				"select max(permission_id) from access_acl join access_permission_rights using(permission_id) join access_resources using(resource_id)"
				" where resource_id=? and identity_id=? and criteria is null", {params[3], params[0]}, sl );
			uint y;
			if( !permissionId ){
				procs.ExecuteStatement( db, "insert into access_permissions( is_role ) values( ? )", {Value{false}}, nullptr, sl );
				permissionId = procs.LastInsertRowId( db );
				procs.ExecuteStatement( db, "insert into access_permission_rights( permission_id, resource_id, allowed, denied ) values( ?, ?, ?, ? )", {Value{*permissionId}, params[3], params[1], params[2]}, nullptr, sl );
				y = procs.ExecuteStatement( db, "insert into access_acl( identity_id, permission_id ) values( ?, ? )", {params[0], Value{*permissionId}}, nullptr, sl );
			}
			else
				y = procs.ExecuteStatement( db, "update access_permission_rights set allowed=?, denied=? where permission_id=?", {params[1], params[2], Value{*permissionId}}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{*permissionId}} } ); //out _permission_id
			return y;
		});
	}
}
