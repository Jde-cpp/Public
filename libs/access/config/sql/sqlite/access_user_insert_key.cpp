#include "accessProcs.h"
#include <jde/db/DBException.h>

#define let const auto

//Twin of ../mysql/access_user_insert_key.sql - the generated access_identity_insert `call` is inlined via IdentityInsert.
//	params: [0]=modulus, [1]=exponent, [2]=provider_id, [3]=name, [4]=target, [5]=description; out _identity_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	//Generated proc twin - see accessProcs.h. Column order/defaults match TableDdl::InsertProcCreateStatement for access_identities.
	α IdentityInsert( IProcs& procs, sqlite3& db, const Value& name, const Value& providerId, const Value& target, const Value& attributes, const Value& description, const Value& isGroup, SL sl )ε->uint{
		procs.ExecuteStatement( db, "insert into access_identities( name, provider_id, target, attributes, created, description, is_group ) values( ?, ?, ?, ?, unixepoch(), ?, ? )", {name, providerId, target, attributes, description, isGroup}, nullptr, sl );
		return procs.LastInsertRowId( db );
	}

	α RegisterAccessUserInsertKey( IProcs& procs )ι->void{
		procs.RegisterProc( "access_user_insert_key", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let emptyString = []( const Value& v ){ return v.is_string() && v.get_string().empty(); };
			THROW_IFSL( emptyString(params[3]) || emptyString(params[4]), "Name and target are required" );
			let identityId = IdentityInsert( procs, db, params[3], params[2], params[4], Value{(uint)0}, params[5], Value{false}, sl );
			let y = procs.ExecuteStatement( db, "insert into access_users( identity_id, modulus, exponent ) values( ?, ?, ? )", {Value{identityId}, params[0], params[1]}, nullptr, sl );
			if( onRow )
				(*onRow)( Row{ {Value{identityId}} } ); //out _identity_id
			return y;
		});
	}
}
