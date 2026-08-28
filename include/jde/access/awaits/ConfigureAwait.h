#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/ql.h>
#include <jde/access/types/Identities.h>

namespace Jde::Access{
	struct AccessListener;
	struct ConfigureAwait : VoidAwait{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, sp<Authorize> authorizer, UserPK executer, sp<AccessListener> listener, string opcServerInstance, bool reload=false, bool allSchemas=false, SRCE )ι:
			VoidAwait{sl}, Authorizer{authorizer}, Executer{executer}, Listener{listener}, OpcServerInstance{move(opcServerInstance)}, QlServer{qlServer}, Schemas{schemas}, Reload{reload}, AllSchemas{allSchemas}{
				ASSERT( listener );
			};
		α Suspend()ι->void override{ if( Reload ) LoadUsers(); else SyncResources(); }
		α SyncResources()ι->VoidTask;
		α LoadUsers()ι->TAwait<Identities>::Task;

		sp<Authorize> Authorizer;
		UserPK Executer;
		sp<AccessListener> Listener;
		string OpcServerInstance;
		sp<QL::IQL> QlServer;
		vector<sp<DB::AppSchema>> Schemas;
		bool Reload;
		//The server's Authorize gates every schema's grants (TestSchemaAdmin) and answers them itself when no OpcServer is
		//registered, so its resource snapshot and its event subscriptions are unfiltered (appserver-review3 #13);  a client -
		//gateway, OpcServer - follows only the schemas it passes.  Schemas still names what ResourceSync creates rows for.
		bool AllSchemas;
	};
}