#include "ApplicationServer.h"
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/ql/QLAwait.h>

namespace Jde::Opc{
	α ApplicationServer::GraphQL( string&& q, UserPK executer, bool returnRaw, SL sl )ι->up<TAwait<jvalue>>{
		return mu<QL::QLAwait<>>( move(q), executer, returnRaw, sl );
	}
	α ApplicationServer::SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<Web::FromServer::SessionInfo>>{
		return mu<App::Client::SessionInfoAwait>( sessionPK, sl );
	}
}