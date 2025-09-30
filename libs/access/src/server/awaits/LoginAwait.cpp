#include <jde/access/server/awaits/LoginAwait.h>
#include <jde/access/usings.h>
#include <jde/fwk/str.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include "../serverInternal.h"

#define let const auto
namespace Jde::Access::Server{
//	α GetSchema()ι->sp<DB::AppSchema>;

	LoginAwait::LoginAwait( Crypto::PublicKey publicKey, string&& name, string&& target, string&& description, SL sl )ι:
			base{sl}, _publicKey{ move(publicKey) }, _name{ move(name) }, _target{ move(target) }, _description{ move(description) }
	{}

	α LoginAwait::LoginTask()ι->TAwait<optional<UserPK::Type>>::Task{
		try{
			let userTable = GetTablePtr( "users" );
			let identityTable = GetTablePtr( "identities" );
			DB::WhereClause where;
			where.Add( userTable->GetColumnPtr("modulus"), DB::Value{_publicKey.ModulusHex()} );
			where.Add( userTable->GetColumnPtr("exponent"), DB::Value{_publicKey.ExponentInt()} );
			where.Add( userTable->GetColumnPtr("provider_id"), DB::Value{underlying(EProviderType::Key)} );
			DB::Statement statement{
				{userTable->GetPK()},
				{ DB::Join{userTable->GetPK(), identityTable->GetPK()} },
				move(where)
			};
			auto sql = statement.Move();
			let userPK = co_await DS().ScalerOpt<UserPK::Type>( move(sql) );
			if( !userPK )
				InsertUser( move(_publicKey.ModulusHex()), _publicKey.ExponentInt() );
			else
				ResumeScaler( {*userPK} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α LoginAwait::InsertUser( string&& modulusHex, uint32_t exponent )ι->DB::ScalerAwait<UserPK::Type>::Task{
		DB::InsertClause insert{ AccessSchema().Prefix+"user_insert_key",
			{ DB::Value{move(modulusHex)}, DB::Value{exponent}, DB::Value{underlying(EProviderType::Key)},
				DB::Value{move(_name)}, DB::Value{move(_target)}, DB::Value{move(_description)}} };
		try{
			let userPK = co_await DS().InsertSeq<UserPK::Type>( move(insert) );
			ResumeScaler( {userPK} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α LoginAwait::Suspend()ι->void{
		LoginTask();
	}
}