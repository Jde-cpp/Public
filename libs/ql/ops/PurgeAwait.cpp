#include "PurgeAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>

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
			optional<jarray> result = co_await Hook::PurgeBefore( _mutation, _userPK );
			auto result0 = result ? result->if_contains( 0 ) : nullptr;
			if( result0 && result0->is_object() && Json::FindDefaultBool(result0->get_object(), "complete") ){
				result0->get_object().erase( "complete" );
				Resume( jarray{move(*result0)} );
			}
			else
				Execute();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α PurgeAwait::Statements( const DB::Table& table )ε->vector<DB::Sql>{
		table.Authorize( Access::ERights::Purge, _userPK, _sl );

		auto pk = table.Extends ? table.SurrogateKeys[0] : table.GetPK();
		DB::Sql sql{
			table.PurgeProcName.size() ? Ƒ( "{}( ? )", table.Schema->Prefix+table.PurgeProcName ) : Ƒ( "delete from {} where {}=?", table.DBName, pk->Name ),
			{ DB::Value{_mutation.AsNumber<uint>("id", _sl)} },
			!table.PurgeProcName.empty()
		};
		vector<DB::Sql> statements{ move(sql) };

		if( table.Extends ){
			let extendedPurge = Statements( AsTable(*table.Extends) );
			statements.insert( end(statements), begin(extendedPurge), end(extendedPurge) );
		}
		return statements;
	}

	α PurgeAwait::Execute()ι->DB::ExecuteAwait::Task{
		try{
			auto statements = Statements( *_table );
			//TODO for mysql allow CLIENT_MULTI_STATEMENTS return ds->Execute( Str::AddSeparators(statements, ";"), parameters, sl );
			uint y{};
			DB::IDataSource& ds = *_table->Schema->DS();
			for( auto& statement : statements )
				y += co_await ds.Execute( move(statement), _sl );
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