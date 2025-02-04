#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Statement.h>
#include "types/MutationQL.h"
#include "types/RequestQL.h"
#include "types/TableQL.h"
#include "types/Subscription.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct IQL;
	α Local()ι->sp<IQL>;

	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;
	//α Mutation( string m, UserPK executer, SRCE )ε->jobject;
	α Parse( string query, bool returnRaw=true, SRCE )ε->RequestQL;
	α ParseSubscriptions( string query, SRCE )ε->vector<Subscription>;
	//α Execute( string query, UserPK executer, SRCE )ε->jobject;
	α Query( const TableQL& table, UserPK executer )ε->jvalue;
	α Query( string query, UserPK executer, SRCE )ε->jvalue;
	α QueryArray( string query, UserPK executer, SRCE )ε->jarray;
	α QueryObject( string query, UserPK executer, SRCE )ε->jobject;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ε->optional<DB::Statement>;

	Ŧ AsId( const jobject& j, SRCE )ε->T;
	Ŧ AsId( const jvalue& j, SRCE )ε->T;
}
namespace Jde{
	Ŧ QL::AsId( const jobject& o, SL sl )ε->T{
		return Json::AsNumber<T>( o, "id", sl );
	}
	Ŧ QL::AsId( const jvalue& v, SL sl )ε->T{
		return AsId<T>( Json::AsObject(v, sl), sl );
	}
}