#pragma once
#include "usings.h"
#include <jde/fwk/co/Await.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/app/proto/App.FromClient.pb.h>
#include <jde/app/proto/App.FromServer.pb.h>
#include <jde/app/proto/Log.pb.h>

namespace Jde::DB{ struct IDataSource; }
namespace Jde::QL{ struct TableQL; }

namespace Jde::App::Server{
	struct ConfigureDSAwait : VoidAwait{
		α Suspend()ι->void override;
	private:
		α EndAppInstances()ι->DB::ExecuteAwait::Task;
		α Configure()ι->VoidAwait::Task;
	};
}
namespace Jde::App{
	α AddConnection( str applicationName, str instanceName, str hostName, uint pid )ε->tuple<ProgramPK, ProgInstPK, ConnectionPK>;
	α EndInstance( ProgInstPK instanceId, SRCE )ι->DB::ExecuteAwait::Task;
}