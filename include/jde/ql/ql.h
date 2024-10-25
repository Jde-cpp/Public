#pragma once
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct MutationQL; struct TableQL;
	using RequestQL=std::variant<vector<TableQL>,MutationQL>;
	//Φ AddMutationListener( string tablePrefix, function<void(const MutationQL& m, uint id)> listener )ι->void;
//	α ParseQL( sv query )ε->RequestQL;
	α Query( sv query, UserPK userId, SRCE )ε->jobject;
	α CoQuery( string query, UserPK userId, SRCE )ι->Coroutine::TPoolAwait<jobject>;
	α Parse( sv query )ε->RequestQL;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ι->void;
}