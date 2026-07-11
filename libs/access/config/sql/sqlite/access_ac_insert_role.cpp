#include <sqlite3.h>
#include "accessProcs.h"

//Twin of ../mysql/access_ac_insert_role.sql.
//	params: [0]=_identityId, [1]=_role_id; no out params.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessAcInsertRole()ι->void{
		RegisterProc( "access_ac_insert_role", []( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			return ExecuteStatement( db, "insert into access_acl( identity_id, permission_id ) values( ?, ? )", {params[0], params[1]}, nullptr, sl );
		});
	}
}
