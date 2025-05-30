#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLAwait.h>

namespace Jde::QL{
	struct IListener;

	struct LocalQL final : IQL{
		α Query( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jvalue>> override{ return mu<QLAwait<jvalue>>( move(query), executer, returnRaw, sl ); }
		α QueryObject( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> override{ return mu<QLAwait<jobject>>( move(query), executer, returnRaw, sl ); }
		α QueryArray( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> override{ return mu<QLAwait<jarray>>( move(query), executer, returnRaw, sl ); }
		α Upsert( string query, UserPK executer )ε->jarray override;
		α Subscribe( string&& query, sp<IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> override;
	};
}