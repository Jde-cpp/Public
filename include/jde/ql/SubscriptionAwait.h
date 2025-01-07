#pragma once
//#include <jde/ql/ql.h>
#include <jde/framework/coroutine/Await.h>
#include "usings.h"

namespace Jde::QL{
	struct IQL;
	struct SubscriptionAwait final : TAwait<vector<SubscriptionId>>{
		SubscriptionAwait( sp<IQL> qlServer, string query, sp<QL::IListener> listener, SubscriptionClientId clientId, UserPK executer, SL sl )ι:
			TAwait<vector<SubscriptionId>>{sl}, _clientId{clientId}, _executer{executer}, _listener{listener},  _qlServer{qlServer}, _query{query}{
				Trace{ ELogTags::Test, "{}", "b" };
			}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<vector<SubscriptionId>>::Task;
		SubscriptionClientId _clientId; UserPK _executer; sp<QL::IListener> _listener; sp<IQL> _qlServer;	string _query;
	};

	struct UnsubscribeAwait final: VoidAwait<>{
		UnsubscribeAwait( vector<uint> ids, sp<IQL> qlServer, SL sl )ι:
			VoidAwait{sl}, _ids{ids}, _qlServer{qlServer}{}
		α await_ready()ι->bool override{ return _ids.empty(); }
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		vector<uint> _ids;
		sp<IQL> _qlServer;
	};
}