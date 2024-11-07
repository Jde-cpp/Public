#include "ServerMock.h"
#include <jde/web/server/IApplicationServer.h>

namespace Jde::Web{
	optional<std::jthread> _webThread;

	struct ApplicationServer final : Server::IApplicationServer{
		β GraphQL( string&&, UserPK, SL )ι->up<TAwait<json>>{ return {}; }
		β SessionInfoAwait( SessionPK, SL )ι->up<TAwait<App::Proto::FromServer::SessionInfo>>{ return {}; }
	};

	α Mock::Start()ι->void{
		Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
	}

	α Mock::Stop()ι->void{
		Server::Stop();
	}
}