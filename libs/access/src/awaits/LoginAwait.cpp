#include <jde/access/awaits/LoginAwait.h>
#include <jde/access/usings.h>
#include <jde/framework/str.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Value.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>

#define let const auto
namespace Jde::Access{
	α GetSchema()ι->sp<DB::AppSchema>;

	LoginAwait::LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SL sl )ι:
			base{sl}, _modulus{ move(modulus) }, _exponent{ move(exponent) }, _name{ move(name) }, _target{ move(target) }, _description{ move(description) }
	{}

	α LoginAwait::LoginTask()ι->Jde::Task{
		auto modulusHex = Str::ToHex( (byte*)_modulus.data(), _modulus.size() );
		try{
			THROW_IF( modulusHex.size() > 1024, "modulus {} is too long. max length: {}", modulusHex.size(), 1024 );
			uint32_t exponent{};
			for( let i : _exponent )
				exponent = ( exponent<<8 ) | i;
			auto schema = GetSchema();
			let userTable = schema->GetViewPtr( "users" );
			let identityTable = schema->GetViewPtr( "identities" );
			DB::WhereClause where;
			where.Add( identityTable->GetColumnPtr("modulus"), DB::Value{modulusHex} );
			where.Add( identityTable->GetColumnPtr("exponent"), DB::Value{exponent} );
			where.Add( userTable->GetColumnPtr("provider_id"), DB::Value{underlying(EProviderType::Key)} );
			DB::Statement statement{
				{userTable->GetPK()},
				{ DB::Join{userTable->GetPK(), identityTable->GetPK()} },
				move(where)
			};
			auto sql = statement.Move();
			auto ds = GetSchema()->DS();
			auto task = ds->ScalerCo<UserPK::Type>( sql.Text, sql.Params );
			auto p = ( co_await task ).UP<UserPK::Type>(); //gcc compile issue
			auto userPK = p ? UserPK{*p} : UserPK{};
			if( !userPK )
				InsertUser( move(modulusHex), exponent, *ds );
			else
				ResumeScaler( userPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α LoginAwait::InsertUser( string&& modulusHex, uint32_t exponent, DB::IDataSource& ds )ι->DB::ScalerAwait<UserPK>::Task{
		DB::InsertClause insert{ "user_insert_key",
			{ DB::Value{move(modulusHex)}, DB::Value{exponent}, DB::Value{underlying(EProviderType::Key)},
				DB::Value{move(_name)}, DB::Value{move(_target)}, DB::Value{move(_description)}} };
		try{
			auto userPK = co_await ds.ExecuteScaler<UserPK>( insert.Move() );
			ResumeScaler( userPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α LoginAwait::Suspend()ι->void{
		LoginTask();
	}
}
