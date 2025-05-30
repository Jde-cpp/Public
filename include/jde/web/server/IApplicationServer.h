#pragma once
#include "usings.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/app/shared/usings.h>

namespace Jde::Web::Server{
	struct IApplicationServer{
		virtual ~IApplicationServer()=default;//msvc warning

		β IsLocal()ι->bool = 0;
		Ω InstancePK()ι->App::AppInstancePK;
		Ω SetInstancePK( App::AppInstancePK x )ι->void;

		β GraphQL( string&& q, UserPK userPK, bool returnRaw=true, SRCE )ι->up<TAwait<jvalue>> =0;
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<Web::FromServer::SessionInfo>> = 0;
	};
}