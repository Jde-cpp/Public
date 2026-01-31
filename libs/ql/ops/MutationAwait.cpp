#include <jde/ql/ops/MutationAwait.h>
#include <jde/ql/IQL.h>
#include "AddRemoveAwait.h"
#include "InsertAwait.h"
#include "PurgeAwait.h"
#include "UpdateAwait.h"

#define let const auto
namespace Jde::QL{
	using namespace DB::Names;

	constexpr ELogTags _tags{ ELogTags::QL };

	MutationAwait::MutationAwait( MutationQL mutation, UserPK executer, sp<IQL> ql, SL sl )ι:
		TAwait<jvalue>{ sl },
		_mutation{ move(mutation) },
		_executer{ executer },
		_ql{ move(ql) }
	{}

	α MutationAwait::Execute()ι->TAwait<jvalue>::Task{

		try{
			jvalue y;
			if( auto await = _ql ? _ql->CustomMutation( _mutation, _executer, _sl ) : nullptr; await )
				y = co_await move(*await);
			else{
				auto table = _mutation.DBTable;
				switch( _mutation.Type ){
				using enum EMutationQL;
				case Update:
				case Delete:
				case Restore:
					y = co_await UpdateAwait{ move(table), move(_mutation), _executer, _sl };
					break;
				case Add:
				case Remove:
					y = co_await AddRemoveAwait{ move(table), move(_mutation), _executer, _sl };
					break;
				case Create:
					y = co_await InsertAwait( move(table), move(_mutation), _executer, _sl );
					break;
				case Purge:
					y = co_await PurgeAwait{ move(table), move(_mutation), _executer, _sl };
					break;
				case Start:
					MutationAwait::Start();
					co_return;
				case Stop:
					MutationAwait::Stop();
					co_return;
				case Execute:
					throw Exception{ ELogTags::QL, _sl, "Execute mutation not implemented." };
				}
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Start()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Start( move(_mutation), _executer );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Stop()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Stop( move(_mutation), _executer );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}