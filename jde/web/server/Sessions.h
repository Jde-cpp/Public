#pragma once
#include <jde/coroutine/Task.h>
#include <jde/db/usings.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

//Holds web session information.  Requires AppServer, so in AppClient
namespace Jde::Web::Server{
	struct SessionInfo;
	namespace Sessions::Internal{ α CreateSession( UserPK userPK, str endpoint, bool isSocket, bool add )ι->sp<SessionInfo>; }
	using namespace Coroutine;
	//using tcp = boost::asio::ip::tcp;
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
		α Add( UserPK userPK, string&& endpoint, bool isSocket )ι->sp<SessionInfo>;
		α Find( SessionPK sessionId )ι->sp<SessionInfo>;
		α Get()ι->vector<sp<SessionInfo>>;
		α Size()ι->uint;

		struct UpsertAwait : TAwait<sp<SessionInfo>>{//TODO rename UpsertSessionAwait or Sessions::UpsertAwait
			using base = TAwait<sp<SessionInfo>>;
			UpsertAwait( str authorization, str endpoint, bool socket, SRCE )ι:base{sl},_authorization{authorization}, _endpoint{endpoint}, _socket{socket}{}
			α await_suspend( Handle h )ι->void;
			α await_resume()ε->sp<SessionInfo>;
		private:
			α Execute()ι->TTask<SessionInfo>;
			string _authorization; string _endpoint; bool _socket;
		};
	}
}