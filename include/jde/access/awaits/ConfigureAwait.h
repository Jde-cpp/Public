#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/ql.h>
#include <jde/access/types/Identities.h>

namespace Jde::Access{
	struct AccessListener;
	struct ConfigureAwait : VoidAwait{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, sp<Authorize> authorizer, UserPK executer, sp<AccessListener> listener, string opcServerInstance, SRCE )ι:
			VoidAwait{sl}, Authorizer{authorizer}, Executer{executer}, Listener{listener}, OpcServerInstance{move(opcServerInstance)}, QlServer{qlServer}, Schemas{schemas}{
				ASSERT( listener );
			};
		α Suspend()ι->void override{ SyncResources(); }
		α SyncResources()ι->VoidTask;
		α LoadUsers()ι->TAwait<Identities>::Task;

		sp<Authorize> Authorizer;
		UserPK Executer;
		sp<AccessListener> Listener;
		string OpcServerInstance;
		sp<QL::IQL> QlServer;
		vector<sp<DB::AppSchema>> Schemas;
	};
}