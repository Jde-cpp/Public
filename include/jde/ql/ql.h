#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Statement.h>
#include "types/MutationQL.h"
#include "types/RequestQL.h"
#include "types/TableQL.h"
#include "types/Subscription.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct Introspection; struct IQL; struct LocalQL;
	α AddIntrospection( Introspection&& x )ι->void;
	Ŧ AsId( const jobject& j, SRCE )ε->T;
	Ŧ AsId( const jvalue& j, SRCE )ε->T;
	α SetSystemMutations( flat_set<string>&& x )ι->void;
	α SetSystemTables( flat_set<string>&& x )ι->void;

	template<class T=uint32> α FindId( const jobject& j )ι->T;
	α Configure( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ε->sp<LocalQL>;
	α Parse( string query, jobject variables, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw=true, SRCE )ε->RequestQL;
	α ParseSubscriptions( string query, jobject variables, const vector<sp<DB::AppSchema>>& schemas, SRCE )ε->vector<Subscription>;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ε->optional<DB::Statement>;
}
namespace Jde{
	Ŧ QL::FindId( const jobject& o )ι->T{
		const auto value = o.try_at( "id" );
		return value ? value->to_number<T>() : T{};
	}
	Ŧ QL::AsId( const jobject& o, SL sl )ε->T{
		return Json::AsNumber<T>( o, "id", sl );
	}
	Ŧ QL::AsId( const jvalue& v, SL sl )ε->T{
		return AsId<T>( Json::AsObject(v, sl), sl );
	}
}