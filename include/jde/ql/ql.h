#pragma once
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/coroutine/Await.h>
#include "types/TableQL.h"
#include "types/MutationQL.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct MutationQL; struct TableQL;
	using RequestQL=std::variant<vector<TableQL>,MutationQL>;

	struct QLAwait final : TAwait<jobject>{
		QLAwait( TableQL&& ql, UserPK userPK, SRCE )ι:TAwait<jobject>{sl},_request{vector{move(ql)}}, _userPK{userPK}{}
		QLAwait( string query, UserPK userPK, SRCE )ε;
		α Suspend()ι->void override;
		α await_resume()ε->jobject override;
	private:
		RequestQL _request;
		UserPK _userPK;
	};

	α Query( string query, UserPK userId, SRCE )ε->jobject;
	α CoQuery( string query, UserPK userId, SRCE )ι->Coroutine::TPoolAwait<jobject>;
	α Parse( string query )ε->RequestQL;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;
}