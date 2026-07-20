#include "accessProcs.h"

#define let const auto

//Twin of the *generated* access_identity_insert proc - column order/defaults match TableDdl::InsertProcCreateStatement
//for access_identities: insertable columns in `i` order, created=$now, the sequence column out last.
//	params: [0]=_name, [1]=_provider_id, [2]=_target, [3]=_attributes, [4]=_description, [5]=_is_group;
//	out _identity_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	α IdentityInsert( IProcs& procs, sqlite3& db, const Value& name, const Value& providerId, const Value& target, const Value& attributes, const Value& description, const Value& isGroup, SL sl )ε->uint{
		procs.ExecuteStatement( db, "insert into access_identities( name, provider_id, target, attributes, created, description, is_group ) values( ?, ?, ?, ?, unixepoch(), ?, ? )", {name, providerId, target, attributes, description, isGroup}, nullptr, sl );
		return procs.LastInsertRowId( db );
	}

	α RegisterAccessIdentityInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "access_identity_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let identityId = IdentityInsert( procs, db, params[0], params[1], params[2], params[3], params[4], params[5], sl );
			if( onRow )
				(*onRow)( Row{ {Value{identityId}} } ); //out _identity_id
			return 1; //ExecuteStatement throws rather than affecting 0 rows.
		}, 6);
	}
}
