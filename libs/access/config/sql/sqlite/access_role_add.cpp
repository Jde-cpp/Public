#include <sqlite3.h>
#include "accessProcs.h"

#define let const auto

//Twin of ../mysql/access_role_add.sql - mysql's null-safe `criteria <=> _criteria` becomes sqlite's `criteria is ?`.
//	params: [0]=_role_id, [1]=_allowed, [2]=_denied, [3]=_resourceTarget, [4]=_schema, [5]=_resourceName, [6]=_criteria;
//	out _permission_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessRoleAdd( IProcs& procs )ι->void{
		procs.RegisterProc( "access_role_add", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			auto resourceId = procs.ScalarUInt( db, "select resource_id from access_resources where target=? and schema_name=coalesce(?, schema_name) and criteria is ?", {params[3], params[4], params[6]}, sl );
			if( !resourceId ){
				procs.ExecuteStatement( db, "insert into access_resources( target, schema_name, name, criteria ) values( ?, ?, coalesce(?, ?), ? )", {params[3], params[4], params[5], params[3], params[6]}, nullptr, sl );
				resourceId = (uint)sqlite3_last_insert_rowid( &db );
			}
			auto permissionId = procs.ScalarUInt( db,
				"select permission_id from access_role_members members join access_permission_rights rights on members.member_id=rights.permission_id"
				" where members.role_id=? and rights.resource_id=?", {params[0], Value{*resourceId}}, sl );
			uint y;
			if( permissionId )
				y = procs.ExecuteStatement( db, "update access_permission_rights set allowed=?, denied=? where permission_id=?", {params[1], params[2], Value{*permissionId}}, nullptr, sl );
			else{
				procs.ExecuteStatement( db, "insert into access_permissions( is_role ) values( ? )", {Value{false}}, nullptr, sl );
				permissionId = (uint)sqlite3_last_insert_rowid( &db );
				procs.ExecuteStatement( db, "insert into access_permission_rights( permission_id, allowed, denied, resource_id ) values( ?, ?, ?, ? )", {Value{*permissionId}, params[1], params[2], Value{*resourceId}}, nullptr, sl );
				y = procs.ExecuteStatement( db, "insert into access_role_members( role_id, member_id ) values( ?, ? )", {params[0], Value{*permissionId}}, nullptr, sl );
			}
			if( onRow )
				(*onRow)( Row{ {Value{*permissionId}} } ); //out _permission_id
			return y;
		});
	}
}
