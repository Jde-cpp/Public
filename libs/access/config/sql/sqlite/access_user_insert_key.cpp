#include "accessProcs.h"
#include "jde/db/generators/Sql.h"
#include "jde/fwk/usings.h"
#include <jde/db/DBException.h>

#define let const auto

//Twin of ../mysql/access_user_insert_key.sql - the generated access_identity_insert `call` is inlined via IdentityInsert.
//	params: [0]=modulus, [1]=exponent, [2]=provider_id, [3]=name, [4]=target, [5]=description, [6]=issuer, [7]=subject_alt, [8]=distinguished, [9]=email, [10]=expiration; out _identity_id returned as the result row.
namespace Jde::DB::Sqlite::AccessProcs{
	constexpr sv _sql = "insert into access_users( identity_id, modulus, exponent, issuer, subject_alt, distinguished, expiration ) values( ?, ?, ?, ?, ?, ?, ? )";
	α RegisterAccessUserInsertKey( IProcs& procs )ι->void{
		procs.RegisterProc( "access_user_insert_key", [&procs](sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl)->uint{
			let emptyString = []( const Value& v ){ return v.is_string() && v.get_string().empty(); };
			THROW_IFSL( emptyString(params[3]) || emptyString(params[4]), "Name and target are required" );
			Sql sql{ string{_sql}, {Value{0}, move(params[0]), move(params[1]), move(params[6]), move(params[7]), move(params[8]), params[10]} };
			THROW_IFX(
				procs.ScalarUInt( db, "select identity_id from access_identities where target=?", {params[4]}, sl ).has_value(),
				DBException( EDbError::App, move(sql), Ƒ("Target '{}' already exists.", params[4].ToString()), {}, sl )
			);
			let identityId = IdentityInsert( procs, db, params[3], params[2], params[4], Value{(uint)0}, params[5], Value{false}, params[9], sl );
			sql.Params[0] = Value{ identityId };
			let y = procs.ExecuteStatement( db, sql.Text, move(sql.Params), nullptr, sl );
			if( onRow )
				( *onRow )( Row{{Value{identityId}}} ); //out _identity_id
			return y;
		}, 11);
	}
}
