#pragma once
#include "usings.h"

namespace Jde::Web::Server{
	struct IApplicationServer{
		virtual ~IApplicationServer()=default;//msvc warning
		β GraphQL( string&& q, UserPK userPK, SRCE )ι->up<TAwait<json>> =0;
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<App::Proto::FromServer::SessionInfo>> = 0;
	};
}