#pragma once
#include "usings.h"
#include <jde/db/awaits/ExecuteAwait.h>

namespace Jde::DB{ struct AppSchema; }

namespace Jde::QL{ struct LocalQL; }
namespace Jde::Access{ struct Authorize; }
namespace Jde::App::Server{
	α InitLogging()ι->void;
	//A host embedding the AppServer beside another role (OpcHub) adds that role's schemas to the QL, the sync and the access
	//snapshot, and supplies the QL over all of them.  Defaults: the AppServer's own two schemas and its AppServerQL.
	struct ConfigureOptions{
		vector<sp<DB::AppSchema>> ExtraSchemas;
		function<sp<QL::LocalQL>(vector<sp<DB::AppSchema>>, sp<Access::Authorize>)> MakeQL;
	};
	//Everything but the listener: db, schema sync, the access snapshot and its listener, this process's connection row, the
	//signing key and the QL hooks.  webServerSettings for its `ssl` (the key the JWTs are signed with).
	α Configure( const jobject& webServerSettings, ConfigureOptions options={} )ε->void;
	α AppStartup( jobject webServerSettings )ε->void;//Configure + the listener + this instance's log settings.
	α AppSchema()ι->sp<DB::AppSchema>;
}
namespace Jde::App{
	α AddConnection( str applicationName, str instanceName, str hostName, uint pid )ε->tuple<ProgramPK, ProgInstPK, ConnectionPK>;
	//ends a *connection*, not a program instance - it stamps connections.deleted for one connection_id.
	α EndConnection( ConnectionPK connectionId, SRCE )ι->DB::ExecuteAwait::Task;
}
