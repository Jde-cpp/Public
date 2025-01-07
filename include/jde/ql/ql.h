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

	α Query( const TableQL& table, UserPK executer )ε->jvalue;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ι->optional<DB::Statement>;
	α Query( string query, UserPK executer, SRCE )ε->jvalue;
	α QueryObject( string query, UserPK executer, SRCE )ε->jobject;
	α QueryArray( string query, UserPK executer, SRCE )ε->jarray;
	α Execute( string query, UserPK executer, SRCE )ε->jobject;
	α Mutation( string m, UserPK executer, SRCE )ε->jobject;
	α Parse( string query )ε->RequestQL;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;

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