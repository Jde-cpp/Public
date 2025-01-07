#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLAwait.h>

namespace Jde::QL{
	struct IListener;

	struct LocalQL final : IQL{
		α Query( string query, UserPK executer, SRCE )ε->up<TAwait<jvalue>> override{ return mu<QLAwait<jvalue>>( move(query), executer, sl ); }
		α QueryObject( string query, UserPK executer, SRCE )ε->up<TAwait<jobject>> override{ return mu<QLAwait<jobject>>( move(query), executer, sl ); }
		α QueryArray( string query, UserPK executer, SRCE )ε->up<TAwait<jarray>> override{ return mu<QLAwait<jarray>>( move(query), executer, sl ); }
		α Subscribe( string&& query, SubscriptionClientId _, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> override;
	};
}
