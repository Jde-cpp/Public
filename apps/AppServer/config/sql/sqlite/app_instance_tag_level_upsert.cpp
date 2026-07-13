#include <sqlite3.h>
#include "appProcs.h"

#define let const auto

//Twin of ../mysql/app_instance_tag_level_upsert.sql - exists-check like the sql version, the table has no unique index for `on conflict`.
//	params: [0]=_instance_id, [1]=_type, [2]=_tag, [3]=_level_id; no out params.
namespace Jde::DB::Sqlite::AppProcs{
	α RegisterAppInstanceTagLevelUpsert( IProcs& procs )ι->void{
		procs.RegisterProc( "app_instance_tag_level_upsert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ*, SL sl )->uint{
			let exists = procs.ScalarUInt( db, "select 1 from app_instance_tag_levels where instance_id=? and tag=? and type=?", {params[0], params[2], params[1]}, sl );
			return exists
				? procs.ExecuteStatement( db, "update app_instance_tag_levels set level_id=? where instance_id=? and tag=? and type=?", {params[3], params[0], params[2], params[1]}, nullptr, sl )
				: procs.ExecuteStatement( db, "insert into app_instance_tag_levels( instance_id, type, tag, level_id ) values( ?, ?, ?, ? )", {params[0], params[1], params[2], params[3]}, nullptr, sl );
		});
	}
}
