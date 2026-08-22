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

	//#25: authorize before the hook, not after it.  Before() co_awaits Hook::PurgeBefore first and the only Authorize was
	//inside Statements(), which runs after - so on the gateway a purge hook had already removed the access provider (and its
	//PurgeFailure counterpart re-created it under a new id) before the caller was refused.  Mirrors UpdateAwait::await_ready:
	//an in-memory acl check needs no suspension, so an unauthorized purge never starts.
	α PurgeAwait::await_ready()ι->bool{
		try{
			THROW_IF( !_table, "Table not found for mutation '{}'.", _mutation.ToString() );
			_table->Authorize( Access::ERights::Purge, _userPK, _sl );
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		return _exception!=nullptr;
	}
	α PurgeAwait::await_resume()ε->jvalue{
		if( _exception )
			_exception->Throw();
		return Promise() ? base::await_resume() : jvalue{};
	}
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
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α PurgeAwait::Statements( const DB::Table& table )ε->vector<DB::Sql>{
		table.Authorize( Access::ERights::Purge, _userPK, _sl );

		auto pk = table.Extends ? table.SurrogateKeys[0] : table.GetPK();
		DB::Sql sql{
			table.PurgeProcName.size() ? Ƒ( "{}( ? )", table.Schema->Prefix+table.PurgeProcName ) : Ƒ( "delete from {} where {}=?", table.SqlName(), pk->Name ),
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
		catch( runtime_error& e ){
			After( ToExceptionPtr(move(e)) );
		}
	}
	α PurgeAwait::After( uint y )ι->MutationAwaits::Task{
		try{
			co_await Hook::PurgeAfter( _mutation, _userPK );
			Resume( jvalue{y} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α PurgeAwait::After( up<runtime_error> e )ι->MutationAwaits::Task{
		try{
			co_await Hook::PurgeFailure( _mutation, _userPK );
			ResumeExp( move(*e) );
		}
		catch( runtime_error& inner ){
			//e->_pInner TODO
			ResumeExp( move(inner) );
		}
	}
	α PurgeAwait::Resume( jvalue&& v )ι->void{
		//#47: a statement that matched nothing is not an event.  OnMutation never looked at the result - an integer rowCount is not
		//an object, so `available` was the args alone and the id the client sent went straight to the listeners.  `deleteUser( id:5,
		//name:"nomatch" )` ands the extra arg into the where clause, updates 0 rows, and still had AccessListener mark user 5
		//deleted in memory - "User is deleted" for every later request by 5, until restart, with the row untouched.
		if( !v.is_number() || v.to_number<uint>() )
			Subscriptions::OnMutation( _mutation, v );
		base::Resume( move(v) );
	}
}