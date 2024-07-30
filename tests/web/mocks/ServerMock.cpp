#include "ServerMock.h"
#include <jde/web/server/IApplicationServer.h>

namespace Jde::Web{
	optional<std::jthread> _webThread;

	struct ApplicationServer final : Server::IApplicationServer{
		β GraphQL( string&& q, UserPK userPK, SRCE )ι->up<TAwait<json>>{ return {}; }
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<Server::SessionInfo>>{ return {}; }
	};

	α Mock::Start()ι->void{
		Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
	}

	α Mock::Stop()ι->void{
		Server::Stop();
	}

}
