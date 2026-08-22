#include <jde/ql/ops/MutationAwait.h>
#include <jde/ql/IQL.h>
#include "AddRemoveAwait.h"
#include "InsertAwait.h"
#include "PurgeAwait.h"
#include "UpdateAwait.h"

#define let const auto
namespace Jde::QL{
	using namespace DB::Names;

	MutationAwait::MutationAwait( MutationQL mutation, QL::Creds creds, sp<IQL> ql, SL sl )ι:
		TAwait<jvalue>{ sl },
		_mutation{ move(mutation) },
		_creds{ move(creds) },
		_ql{ move(ql) }
	{}

	α MutationAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			jvalue y;
			if( auto await = _ql ? _ql->CustomMutation( _mutation, _creds, _sl ) : nullptr; await )
				y = co_await move(*await);
			else{
				_mutation.CheckVariables( _sl );
				auto table = _mutation.DBTable;
				using enum EMutationQL;
				//MutationQL leaves DBTable null for system-shaped or empty names ("createStatus", "create"), and the crud ops read
				//through it before they authorize - View::Authorize dereferences `this`.  Start/Stop are hook-implemented and resolve
				//no table, and Execute has its own message, so the guard can not go above the switch.  Mirrors UpdateAwait::await_ready.
				if( let type=_mutation.Type; type!=Start && type!=Stop && type!=Execute )
					THROW_IF( !table, "Table not found for mutation '{}'.", _mutation.ToString() );
				switch( _mutation.Type ){
				case Update:
				case Delete:
				case Restore:
					y = co_await UpdateAwait{ move(table), move(_mutation), _creds.UserPK(), _sl };
					break;
				case Add:
				case Remove:
					y = co_await AddRemoveAwait{ move(table), move(_mutation), _creds.UserPK(), _sl };
					break;
				case Create:
					y = co_await InsertAwait( move(table), move(_mutation), _creds.UserPK(), _sl );
					break;
				case Purge:
					y = co_await PurgeAwait{ move(table), move(_mutation), _creds.UserPK(), _sl };
					break;
				case Start:
					MutationAwait::Start();
					co_return;
				case Stop:
					MutationAwait::Stop();
					co_return;
				case Execute:
					throw Exception{ "Execute mutation not implemented.", {ELogTags::QL}, _sl };
				}
			}
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Start()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Start( _mutation, _creds.UserPK() );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Stop()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Stop( _mutation, _creds.UserPK() );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}