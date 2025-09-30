#pragma once
#include <jde/db/usings.h>
#include <jde/fwk/co/Await.h>
#include <jde/fwk/co/Task.h>
#include "exports.h"

#define Φ auto ΓWS
namespace Jde::App{ struct IApp; }
namespace Jde::Web{ struct Jwt; }
namespace Jde::Web::FromServer{ struct SessionInfo; }
//Holds web session information.
namespace Jde::Web::Server{
	struct SessionInfo;
	namespace Sessions::Internal{ α CreateSession( UserPK userPK, str endpoint, bool isSocket, bool add )ι->sp<SessionInfo>; }
	struct SessionInfo{
		SessionInfo()ι=default;
		SessionInfo( SessionPK sessionPK, steady_clock::time_point expiration, Jde::UserPK userPK, str userEndpointAddress, bool hasSocket )ι;

		SessionPK SessionId;
		Jde::UserPK UserPK;
		string UserEndpoint;
		bool HasSocket;//before Expiration
		steady_clock::time_point Expiration;
		steady_clock::time_point LastServerUpdate;
		bool IsInitialRequest{};
		α NewExpiration()Ι->steady_clock::time_point;
	private:
		SessionInfo( SessionPK sessionPK, str UserEndpoint, bool hasSocket )ι;
		friend α Sessions::Internal::CreateSession( Jde::UserPK, str, bool, bool )ι->sp<SessionInfo>;
  };

	//TODO - change to GraphQL.
	namespace Sessions{
		Φ Add( UserPK userPK, string&& endpoint, bool isSocket )ι->sp<SessionInfo>;
		Φ Find( SessionPK sessionId )ι->sp<SessionInfo>;
		Φ Remove( SessionPK sessionId )ι->bool;
		α RestSessionTimeout()ι->steady_clock::duration;
		α Get()ι->vector<sp<SessionInfo>>;
		α Size()ι->uint;

		struct ΓWS UpsertAwait : TAwait<sp<SessionInfo>>{
			using base = TAwait<sp<SessionInfo>>;
			UpsertAwait( str authorization, str endpoint, bool socket, sp<App::IApp> appClient, bool throw_=true, SRCE )ι:base{sl}, _appClient{move(appClient)}, _authorization{authorization}, _endpoint{endpoint}, _socket{socket}, _throw{throw_}{}
			α Suspend()ι->void;
			α await_resume()ε->sp<SessionInfo>;
		private:
			α Execute()ι->TTask<Web::FromServer::SessionInfo>;
			α FromSessionId()ι->TTask<Web::FromServer::SessionInfo>;
			α FromJwt( str jwt )ι->TTask<UserPK>;
			α CreateSession( UserPK userPK={} )ι->void;
			sp<App::IApp> _appClient; string _authorization; string _endpoint; bool _socket; bool _throw;
		};
	}
}
#undef Φ