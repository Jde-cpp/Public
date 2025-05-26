#pragma once
#include <jde/ql/ql.h>
//#include <jde/ql/SubscriptionAwait.h>
#include "../accessInternal.h"

namespace Jde::Access{
	struct EventTypeSubscribeAwait : VoidAwait<>{
		EventTypeSubscribeAwait( sp<QL::IQL> qlServer, string name, ESubscription type, sv cols, ESubscription events, UserPK executer, SRCE )ι:
			VoidAwait{sl}, _qlServer{qlServer}, _name{ move(name) }, _type{ type }, _cols{ cols }, _events{ events }, _executer{executer} {}
	private:
		α Suspend()ι->void override{ Subscribe(); }
		α Subscribe()ι->TAwait<vector<QL::SubscriptionId>>::Task;
		sp<QL::IQL> _qlServer; string _name; ESubscription _type; sv _cols; ESubscription _events; UserPK _executer;
	};
	struct EventsSubscribeAwait : VoidAwait<>{
		EventsSubscribeAwait( sp<QL::IQL> qlServer, UserPK executer, SRCE )ι:
			VoidAwait{sl}, _qlServer{qlServer}, _executer{ executer }{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->EventTypeSubscribeAwait::Task;
		sp<QL::IQL> _qlServer; UserPK _executer;
	};
}