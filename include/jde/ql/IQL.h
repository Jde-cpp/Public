#include "UnsubscribeAwait.h"

namespace Jde::QL{
	struct IQL : std::enable_shared_from_this<IQL>{
		[[nodiscard]] β Query( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jvalue>> =0;
		[[nodiscard]] β QueryObject( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> =0;
		[[nodiscard]] β QueryArray( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> =0;
		β Upsert( string query, UserPK executer )ε->jarray=0;

		[[nodiscard]] α Unsubscribe( flat_set<SubscriptionId>&& ids, SRCE )ε->UnsubscribeAwait{ return UnsubscribeAwait{ move(ids), shared_from_this(), sl }; }
		[[nodiscard]] β Subscribe( string&& query, sp<IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> = 0;
	};
}