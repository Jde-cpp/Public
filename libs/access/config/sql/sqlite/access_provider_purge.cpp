#include <sqlite3.h>
#include "accessProcs.h"

//Twin of ../mysql/access_provider_purge.sql.
//	params: [0]=_provider_id; no out params.
namespace Jde::DB::Sqlite::AccessProcs{
	α RegisterAccessProviderPurge()ι->void{
		RegisterProc( "access_provider_purge", []( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			ExecuteStatement( db, "delete from access_users where identity_id in ( select identity_id from access_identities where provider_id=? )", {params[0]}, nullptr, sl );
			ExecuteStatement( db, "delete from access_identities where provider_id=?", {params[0]}, nullptr, sl );
			return ExecuteStatement( db, "delete from access_providers where provider_id=?", {params[0]}, nullptr, sl );
		});
	}
}
