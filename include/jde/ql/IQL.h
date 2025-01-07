#include "SubscriptionAwait.h"

namespace Jde::QL{
	struct IQL : std::enable_shared_from_this<IQL>{
		[[nodiscard]] β Query( string query, UserPK executer, SRCE )ε->up<TAwait<jvalue>> =0;
		[[nodiscard]] β QueryObject( string query, UserPK executer, SRCE )ε->up<TAwait<jobject>> =0;
		[[nodiscard]] β QueryArray( string query, UserPK executer, SRCE )ε->up<TAwait<jarray>> =0;
		//Sets up listener and calls query.
		//For remote, sets requestId=clientid.
		[[nodiscard]] β Subscribe( string&& query, SubscriptionClientId clientId, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> =0;
		[[nodiscard]] α Unsubscribe( vector<uint>&& ids, SRCE )ε->UnsubscribeAwait{ return UnsubscribeAwait{ move(ids), shared_from_this(), sl }; }
		[[nodiscard]] α Subscribe( string&& query, sp<IListener> listener, SubscriptionClientId clientId, UserPK executer, SRCE )ε->SubscriptionAwait{
			return SubscriptionAwait{ shared_from_this(), move(query), listener, clientId, executer, sl };
		}
	};
}