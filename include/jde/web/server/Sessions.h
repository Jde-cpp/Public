#pragma once
#include <jde/db/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/Task.h>
#include "exports.h"

#define Φ auto ΓWS
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
		α Get()ι->vector<sp<SessionInfo>>;
		α Size()ι->uint;

		struct ΓWS UpsertAwait : TAwait<sp<SessionInfo>>{//TODO rename UpsertSessionAwait or Sessions::UpsertAwait
			using base = TAwait<sp<SessionInfo>>;
			UpsertAwait( str authorization, str endpoint, bool socket, SRCE )ι:base{sl},_authorization{authorization}, _endpoint{endpoint}, _socket{socket}{}
			α Suspend()ι->void;
			α await_resume()ε->sp<SessionInfo>;
		private:
			α Execute()ι->TTask<App::Proto::FromServer::SessionInfo>;
			string _authorization; string _endpoint; bool _socket;
		};
	}
}
#undef Φ