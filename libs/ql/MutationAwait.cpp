#include "MutationAwait.h"
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
//#include <jde/db/generators/UpdateStatement.h>
#include "ops/AddRemoveAwait.h"
#include "ops/InsertAwait.h"
#include "ops/PurgeAwait.h"
#include "ops/UpdateAwait.h"
#include "types/QLColumn.h"

#define let const auto
namespace Jde::QL{
	using namespace Coroutine;
	using namespace DB::Names;

	constexpr ELogTags _tags{ ELogTags::QL };
	α GetTable( str tableName, SRCE )ε->sp<DB::View>;

	MutationAwait::MutationAwait( MutationQL mutation, UserPK userPK, SL sl )ι:
		TAwait<jvalue>{ sl },
		_mutation{ move(mutation) },
		_userPK{ userPK }
	{}

	α MutationAwait::Execute()ι->TAwait<jvalue>::Task{
		let& table = DB::AsTable( GetTable(_mutation.TableName()) );
		jvalue y;
		try{
			switch( _mutation.Type ){
			using enum EMutationQL;
			case Update:
			case Delete:
			case Restore:
				y = co_await UpdateAwait{ table, move(_mutation), _userPK, _sl };
				break;
			case Add:
			case Remove:
				y = co_await AddRemoveAwait{ table, move(_mutation), _userPK, _sl };
				break;
			case Create:
				y = co_await InsertAwait( table, move(_mutation), _userPK, _sl );
				//Trace{ _tags, "MutationAwait::Execute - Create '{}'", serialize(y) };
				break;
			case Purge:
				y = co_await PurgeAwait{ table, move(_mutation), _userPK, _sl };
				break;
			case Start:
				MutationAwait::Start();
				co_return;
			case Stop:
				MutationAwait::Stop();
				co_return;
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Start()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Start( move(_mutation), _userPK );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α MutationAwait::Stop()ι->MutationAwaits::Task{
		try{
			optional<jvalue> y = co_await Hook::Stop( move(_mutation), _userPK );
			Resume( y ? move(*y) : jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}