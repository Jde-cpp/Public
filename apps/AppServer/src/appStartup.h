#pragma once
#include "usings.h"
#include <jde/db/awaits/ExecuteAwait.h>

namespace Jde::DB{ struct AppSchema; }

namespace Jde::App::Server{
	α InitLogging()ι->void;
	α AppStartup( jobject webServerSettings )ε->void;
	α AppSchema()ι->sp<DB::AppSchema>;
}
namespace Jde::App{
	α AddConnection( str applicationName, str instanceName, str hostName, uint pid )ε->tuple<ProgramPK, ProgInstPK, ConnectionPK>;
	//ends a *connection*, not a program instance - it stamps connections.deleted for one connection_id.
	α EndConnection( ConnectionPK connectionId, SRCE )ι->DB::ExecuteAwait::Task;
}
