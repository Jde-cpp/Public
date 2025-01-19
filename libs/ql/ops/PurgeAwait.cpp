#include "PurgeAwait.h"
#include <jde/db/Database.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include "../GraphQuery.h"

#define let const auto

namespace Jde::QL{
	PurgeAwait::PurgeAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SL sl )ι:
		base{ sl },
		_mutation{ move(mutation) },
		_table{ table },
		_userPK{ userPK }
	{}

	α PurgeAwait::Before()ι->MutationAwaits::Task{
		try{
			co_await Hook::PurgeBefore( _mutation, _userPK );
			Execute();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
			co_return;
		}
	}
	α PurgeAwait::Statements( const DB::Table& table, vector<DB::Value>& parameters )ε->vector<string>{
		let id = Json::AsNumber<uint>( _mutation.Args, "id" );
		parameters.push_back( DB::Value{DB::EType::ULong, id} );
		table.Authorize( Access::ERights::Purge, _userPK, _sl );

		auto pk = table.Extends ? table.SurrogateKeys[0] : table.GetPK();
		vector<string> statements{ table.PurgeProcName.size() ? Ƒ("{}( ? )", table.Schema->Prefix+table.PurgeProcName) : Ƒ("delete from {} where {}=?", table.DBName, pk->Name) };
		if( table.Extends ){
			let extendedPurge = Statements( AsTable(*table.Extends), parameters );
			statements.insert( end(statements), begin(extendedPurge), end(extendedPurge) );
		}
		return statements;
	}

	α PurgeAwait::Execute()ι->Coroutine::Task{
		try{
			vector<DB::Value> parameters;
			auto statements = Statements( *_table, parameters );
			//TODO for mysql allow CLIENT_MULTI_STATEMENTS return ds->Execute( Str::AddSeparators(statements, ";"), parameters, sl );
			uint y;
			DB::IDataSource& ds = *_table->Schema->DS();
			for( auto& statement : statements ){
				auto a = statement.starts_with("delete ")
					? ds.ExecuteCo(move(statement), vector<DB::Value>{parameters.front()}, _sl) //right now, parameters should singular and the same.
					: ds.ExecuteProcCo( move(statement), move(parameters), _sl );
				y += *( co_await *a ).UP<uint>(); //gcc compiler error.
			}
			After( y );
		}
		catch( IException& e ){
			After( e.Move() );
		}
	}
	α PurgeAwait::After( uint y )ι->MutationAwaits::Task{
		try{
			co_await Hook::PurgeAfter( _mutation, _userPK );
			Resume( jvalue{y} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α PurgeAwait::After( up<IException>&& e )ι->MutationAwaits::Task{
		try{
			co_await Hook::PurgeFailure( _mutation, _userPK );
			ResumeExp( move(*e) );
		}
		catch( IException& inner ){
			//e->_pInner TODO
			ResumeExp( move(inner) );
		}
	}
	α PurgeAwait::Resume( jvalue&& v )ι->void{
		Subscriptions::OnMutation( _mutation, v );
		base::Resume( move(v) );
	}
}