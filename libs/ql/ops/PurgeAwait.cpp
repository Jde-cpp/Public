#include "PurgeAwait.h"
#include <jde/access/IAcl.h>
//#include "../GraphQL.h"
#include <jde/ql/GraphQLHook.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/db/Database.h>
#include <jde/db/meta/Column.h>
#include "../GraphQuery.h"

#define let const auto

namespace Jde::QL{
	PurgeAwait::PurgeAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK, SL sl )ι:
		AsyncAwait{ [&,user=userPK](HCoroutine h){ Execute( table, mutation, user, h ); }, sl, "PurgeAwait" }
	{}

	α PurgeStatements( const DB::Table& table, const MutationQL& m, UserPK userPK, vector<DB::Value>& parameters, SRCE )ε->vector<string>{
		let id = Json::AsNumber<uint>( m.Args, "id");
		parameters.push_back( DB::Value{DB::EType::ULong, id} );
		if( let p=table.Schema->Authorizer; p )
			p->Test(Access::ERights::Purge, table.Name, userPK);

		auto pk = table.Extends ? table.SurrogateKeys[0] : table.GetPK();
		vector<string> statements{ table.PurgeProcName.size() ? Ƒ("{}( ? )", table.PurgeProcName) : Ƒ("delete from {} where {}=?", table.DBName, pk->Name) };
		if( table.Extends ){
			let extendedPurge = PurgeStatements( AsTable(*table.Extends), m, userPK, parameters, sl );
			statements.insert( end(statements), begin(extendedPurge), end(extendedPurge) );
		}
		return statements;
	}

	α PurgeAwait::Execute( const DB::Table& table, MutationQL mutation, UserPK userPK, HCoroutine h )ι->Task{
		try{
			( co_await Hook::PurgeBefore(mutation, userPK) ).CheckError();
		}
		catch( IException& e ){
			Resume( move(e), move(h) );
			co_return;
		}
		bool success{ true };
		try{
		vector<DB::Value> parameters;
		auto statements = PurgeStatements( table, mutation, userPK, parameters, _sl );

			//TODO for mysql allow CLIENT_MULTI_STATEMENTS return ds->Execute( Str::AddSeparators(statements, ";"), parameters, sl );
			auto result{ mu<uint>() };
			sp<DB::IDataSource> ds = table.Schema->DS();
			for( auto& statement : statements ){
				auto a = statement.starts_with("delete ")
					? ds->ExecuteCo(move(statement), vector<DB::Value>{parameters.front()}, _sl) //right now, parameters should singular and the same.
					: ds->ExecuteProcCo( move(statement), move(parameters), _sl );
			 	*result += *( co_await *a ).UP<uint>(); //gcc compiler error.
			}
			Resume( move(result), move(h) );
		}
		catch( IException& ){
			success = false;
		}
		if( !success )
			co_await Hook::PurgeFailure( mutation, userPK );
	}
}