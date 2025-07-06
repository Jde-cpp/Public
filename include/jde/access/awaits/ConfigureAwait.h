#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>

namespace Jde::Access{
	struct AccessListener;
	struct ConfigureAwait : VoidAwait<>{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, sp<Authorize> authorizer, UserPK executer, sp<AccessListener> listener, SRCE )ι:
			VoidAwait<>{sl}, Authorizer{authorizer}, Executer{executer}, QlServer{qlServer}, Schemas{schemas}, Listener{listener}{};
		α Suspend()ι->void override;

		sp<Authorize> Authorizer;
		UserPK Executer;
		sp<QL::IQL> QlServer;
		vector<sp<DB::AppSchema>> Schemas;
		sp<AccessListener> Listener;
	};
}