#include "AppClient.h"
#include <jde/app/client/appClient.h>
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/ql/IQL.h>

#include "StartupAwait.h"

namespace Jde::Opc::Gateway{
	α AppClient::GraphQL( string&& q, UserPK executer, bool returnRaw, SL sl )ι->up<TAwait<jvalue>>{
		return App::Client::QLServer()->Query( move(q), executer, returnRaw, sl );
	}
	α AppClient::SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<Web::FromServer::SessionInfo>>{
		return mu<App::Client::SessionInfoAwait>( sessionPK, sl );
	}
}