#pragma once
#include "usings.h"
#include <jde/framework/coroutine/Await.h>

namespace Jde::Web::Server{
	struct IApplicationServer{
		virtual ~IApplicationServer()=default;//msvc warning
		β GraphQL( string&& q, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> =0;
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<Web::FromServer::SessionInfo>> = 0;
	};
}