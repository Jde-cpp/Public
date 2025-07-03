#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Statement.h>
//#include "QLAwait.h"
#include "types/MutationQL.h"
#include "types/RequestQL.h"
#include "types/TableQL.h"
#include "types/Subscription.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct IQL;
	Ŧ AsId( const jobject& j, SRCE )ε->T;
	Ŧ AsId( const jvalue& j, SRCE )ε->T;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;
	α Local()ι->sp<IQL>;
	α Parse( string query, bool returnRaw=true, SRCE )ε->RequestQL;
	α ParseSubscriptions( string query, SRCE )ε->vector<Subscription>;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ε->optional<DB::Statement>;
}
namespace Jde{
	Ŧ QL::AsId( const jobject& o, SL sl )ε->T{
		return Json::AsNumber<T>( o, "id", sl );
	}
	Ŧ QL::AsId( const jvalue& v, SL sl )ε->T{
		return AsId<T>( Json::AsObject(v, sl), sl );
	}
}