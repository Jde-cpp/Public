#pragma once
#include <jde/framework/coroutine/Await.h>
#include "usings.h"

namespace Jde::QL{
	struct IQL;
	struct UnsubscribeAwait final: VoidAwait{
		UnsubscribeAwait( flat_set<SubscriptionId>&& ids, sp<IQL> qlServer, SL sl )ι:
			VoidAwait{sl}, _ids{move(ids)}, _qlServer{qlServer}{}
		α await_ready()ι->bool override{ return _ids.empty(); }
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		flat_set<SubscriptionId> _ids;
		sp<IQL> _qlServer;
	};
}