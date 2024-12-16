#include <jde/access/awaits/AuthenticateAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Value.h>
#include <jde/db/generators/InsertClause.h>
#include "../accessInternal.h"

#define let const auto
namespace Jde::Access{
	α AuthenticateAwait::InsertUser( vector<DB::Value>&& params )->TAwait<UserPK::Type>::Task{
		try{
			let userPK = co_await DS()->ExecuteScaler<UserPK::Type>( DB::InsertClause{ "user_insert_login", move(params) }.Move() );
			ResumeScaler( {userPK} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AuthenticateAwait::Execute()ι->Jde::Task{
		let identities = GetTable( "identities" );
		let identityPK = identities->GetColumnPtr( "identity_id" );
		let providerFK = identities->GetColumnPtr( "provider_id" );
		DB::SelectClause select{ identityPK };
		DB::FromClause from;
		let usersTable = GetTable( "users" );
		from.TryAdd( {identityPK, usersTable->GetPK(), true} );
		let providers = GetTable( "providers" );
		from.TryAdd( {providerFK, providers->GetPK(), true} );
		DB::WhereClause where;
		where.Add( usersTable->GetColumnPtr("login_name"), _loginName );
		where.Add( providerFK, _providerId );
		auto targetColumn = providers->GetColumnPtr("target");
		if( _opcServer.size() )
			where.Add( targetColumn, _opcServer );
		else
			where.Add( targetColumn, nullptr );
		auto sql = DB::Statement{ move(select), move(from), move(where) }.Move();
		try{
			auto task = DS()->ScalerCo<UserPK::Type>( sql.Text, move(sql.Params) );
			auto userPK = ( co_await task ).UP<UserPK::Type>(); //gcc compile issue
			//auto userPK = p ? *p : UserPK{};
			if( !userPK ){
				if( _opcServer.empty() )
					sql.Params.emplace_back( nullptr );
				InsertUser( move(sql.Params) );
				co_return;
			}
			ResumeScaler( {*userPK} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}