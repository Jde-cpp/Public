#pragma once
#include "usings.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>

namespace Jde::DB{ struct IDataSource; }
namespace Jde::QL{ struct TableQL; }

namespace Jde::App::Server{
	struct ConfigureDSAwait : VoidAwait{
		α Suspend()ι->void override;
	private:
		α EndAppInstances()ι->DB::ExecuteAwait::Task;
		α Configure()ι->VoidAwait::Task;
	};
	α SaveString( App::Proto::FromClient::EFields field, StringMd5 id, string value, SRCE )ι->void;
}
namespace Jde::App{
	α AddInstance( str applicationName, str hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK>;
	α EndInstance( AppInstancePK instanceId, SRCE )ι->DB::ExecuteAwait::Task;

	//α LoadApplications( AppPK pk=0 )ι->up<Proto::FromServer::Applications>;
	namespace Data{
		α LoadEntries( QL::TableQL table )ε->Proto::FromServer::Traces;
		α LoadStrings( SRCE )ε->void;
	}
	α SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, SRCE )ι->void;
}