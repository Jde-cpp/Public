#pragma once

namespace Jde::Web::Server{
	struct SessionInfo;
	struct IApplicationServer{
		β GraphQL( string&& q, UserPK userPK, SRCE )ι->up<TAwait<json>> =0;
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<SessionInfo>> = 0;
	};
}
