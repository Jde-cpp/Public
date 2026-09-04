#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/access/awaits/ConfigureAwait.h>

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{  struct IQL; }
namespace Jde::Access{ struct Authorize; struct AccessListener; }

namespace Jde::Access::Client{
	//What a client's access snapshot was built from, so a reconnect can rebuild it on the new session.  Owned by the client
	//that owns the authorizer (IAppClient) - it was a file-scope static here, which handed every app client in one process the
	//last caller's context (opcserver-review3 #16).
	struct Context final{
		vector<sp<DB::AppSchema>> Schemas;//the schemas this client follows - its own, not every schema (ConfigureAwait::AllSchemas).
		sp<Authorize> Authorizer;
		UserPK Executer;
		sp<AccessListener> Listener;
		string ResourceSchema;//the OpcServer's instance suffix, "" elsewhere - names its `opc.<instance>` resource rows.
	};
	α Configure( sp<DB::AppSchema> accessSchema, const Context& context, sp<QL::IQL> appQL, bool syncOnly=false, SRCE )ε->ConfigureAwait;//syncOnly: ConfigureAwait::SyncOnly - an in-process client that shares the server's Authorize.
	α Reload( const Context& context, sp<QL::IQL> appQL, SRCE )ε->ConfigureAwait;//appQL is the new session's ClientQL - the one Configure was handed died with the old session.
}
