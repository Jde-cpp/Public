#include <jde/ql/ops/MutationAwait.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLHook.h>
#include <jde/fwk/co/AnyAwait.h>
#include <jde/fwk/io/Cache.h>
#include <jde/db/meta/Table.h>
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
				//through it before they authorize - Table::Authorize dereferences `this`.  Start/Stop are hook-implemented and resolve
				//no table, and Execute has its own message, so the guard can not go above the switch.  Mirrors UpdateAwait::await_ready.
				if( let type=_mutation.Type; type!=Start && type!=Stop && type!=Execute )
					THROW_IF( !table, "Table not found for mutation '{}'.", _mutation.ToString() );
				let enumCache = table && (table->IsEnum() || table->IsFlags) ? table->Name : string{};
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
					y = co_await InsertAwait( move(table), move(_mutation), _creds.UserPK(), false, _sl );
					break;
				case Purge:
					y = co_await PurgeAwait{ move(table), move(_mutation), _creds.UserPK(), _sl };
					break;
				case Start:
				case Stop:{//hook-implemented, no table; the MutationAwaits is awaitable from this frame, so the two twin coroutines are a pointer pick.
					auto ask = _mutation.Type==Start ? &Hook::Start : &Hook::Stop;
					auto result = co_await ask( _mutation, _creds.UserPK(), _sl );
					y = result ? jvalue{ move(*result) } : jvalue{};
					break;
				}
				case Execute:
					throw Exception{ "Execute mutation not implemented.", {ELogTags::QL}, _sl };
				}
				if( enumCache.size() )
					Cache::Clear( enumCache );
			}
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}