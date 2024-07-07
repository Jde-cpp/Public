#pragma once
#include <jde/coroutine/Task.h>
#include <jde/db/usings.h>

namespace Jde::App::Client{
	using namespace Coroutine;
	using tcp = boost::asio::ip::tcp;
	struct SessionInfo{
		SessionInfo()ι=default;
		SessionInfo( SessionPK sessionPK, str UserEndpoint, bool hasSocket )ι;
		SessionInfo( SessionPK sessionPK, steady_clock::time_point expiration, Jde::UserPK userPK, str userEndpointAddress, bool hasSocket )ι;

		SessionPK SessionId;
		Jde::UserPK UserPK;
		string UserEndpoint;
		bool HasSocket;//before Expiration
		steady_clock::time_point Expiration;
		steady_clock::time_point LastServerUpdate;
		bool IsInitialRequest{};
		α NewExpiration()Ι->steady_clock::time_point;
  };

	//TODO - change to GraphQL.
	α FindSession( SessionPK sessionId )ι->optional<SessionInfo>;
	α GetSessions()ι->vector<SessionInfo>;
	//TODO TAwait<SessionInfo>
	struct UpsertAwait{
		using Task = TTask<SessionInfo>; using Promise = Task::promise_type; using Handle = coroutine_handle<Promise>;
		UpsertAwait( str authorization, str endpoint, bool socket, SRCE )ι:_authorization{authorization}, _endpoint{endpoint}, _socket{socket}, _sl{sl}{}
		α await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void;
		α await_resume()ε->SessionInfo;
	private:
		string _authorization; string _endpoint; bool _socket; SL _sl; Promise* _promise;
	};
}