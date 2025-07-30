#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/access/types/Identities.h>

namespace Jde::Access{
	struct AccessListener;
	struct ConfigureAwait : VoidAwait{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, sp<Authorize> authorizer, UserPK executer, sp<AccessListener> listener, SRCE )ι:
			VoidAwait{sl}, Authorizer{authorizer}, Executer{executer}, QlServer{qlServer}, Schemas{schemas}, Listener{listener}{
				ASSERT( listener );
			};
		α Suspend()ι->void override{ SyncResources(); }
		α SyncResources()ι->VoidTask;
		α LoadUsers()ι->TAwait<Identities>::Task;

		sp<Authorize> Authorizer;
		UserPK Executer;
		sp<QL::IQL> QlServer;
		vector<sp<DB::AppSchema>> Schemas;
		sp<AccessListener> Listener;
	};
}