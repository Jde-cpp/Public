#include "AppServer.h"
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/ql/QLAwait.h>

namespace Jde::Opc::Server{
	α AppServer::GraphQL( string&& q, UserPK executer, bool returnRaw, SL sl )ι->up<TAwait<jvalue>>{
		return mu<QL::QLAwait<>>( move(q), executer, returnRaw, sl );
	}
	α AppServer::SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<Web::FromServer::SessionInfo>>{
		return mu<App::Client::SessionInfoAwait>( sessionPK, sl );
	}
}