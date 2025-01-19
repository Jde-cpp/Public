#include "LocalQL.h"
#include <jde/ql/ql.h>
#include <jde/ql/LocalSubscriptions.h>
//#include <jde/ql/UnsubscribeAwait.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };

	struct SubscribeQueryAwait : TAwait<vector<SubscriptionId>>{
		using Await = TAwait<jobject>;
		using base = TAwait<vector<SubscriptionId>>;
		SubscribeQueryAwait( vector<Subscription>&& sub, sp<IListener> listener, UserPK executer, SRCE )ι:
			base{sl}, _executer{executer}, _listener{listener}, _subscriptions{move(sub)}{}
		α await_ready()ι->bool override{
			for_each( _subscriptions, [&]( Subscription& sub ){_result.push_back(sub.Id);} );
			Subscriptions::Listen( _listener, move(_subscriptions) );
			return true;
		}
		α Suspend()ι->void override{}
		α await_resume()ι->vector<SubscriptionId> override{ return _result; }
	private:
		UserPK _executer;
		sp<IListener> _listener;
		vector<Subscription> _subscriptions;
		vector<SubscriptionId> _result;
	};
	α LocalQL::Subscribe( string&& query, sp<IListener> listener, UserPK executer, SL sl )ε->up<TAwait<vector<SubscriptionId>>>{
		return mu<SubscribeQueryAwait>( ParseSubscriptions(move(query)), listener, executer, sl );
	}
}