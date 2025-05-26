#include "ServerMock.h"
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/Server.h>

namespace Jde::Web{
	optional<std::jthread> _webThread;

	struct ApplicationServer final : Server::IApplicationServer{
		α IsLocal()ι->bool{ return true; }
		α GraphQL( string&&, UserPK, bool, SL )ι->up<TAwait<jvalue>>{ return {}; }
		α SessionInfoAwait( SessionPK, SL )ι->up<TAwait<Web::FromServer::SessionInfo>>{ return {}; }
	};

	α Mock::Start()ι->void{
		Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
	}

	α Mock::Stop()ι->void{
		Server::Stop();
	}
}