#pragma once
//#include "HttpRequestAwait.h"
#include "../usings.h"

namespace Jde::Web::Flex::Sessions{

	struct Info{
		Info()ι=default;
		Info( SessionPK sessionPK, const tcp::endpoint& UserEndpoint, bool isSocket )ι;
		Info( const SessionInfo& proto, const tcp::endpoint& userEndpoint, bool isSocket )ι:SessionId{proto.session_id()}, UserPK{proto.user_id()}, UserEndpoint{userEndpoint}, IsSocket{isSocket}{}
//		α operator==(const Info& b)const{ return SessionId==b.SessionId && UserEndpoint==b.UserEndpoint; }
//		struct HashFunction{ α operator()(const Info& x)const{ return std::hash<SessionPK>()(x.SessionId) ^ std::hash<tcp::endpoint>()(x.UserEndpoint); } };

		SessionPK SessionId;
		Jde::UserPK UserPK;
		tcp::endpoint UserEndpoint;
		bool IsSocket;//before Expiration
		steady_clock::time_point Expiration;
		steady_clock::time_point LastServerUpdate;
		bool IsInitialRequest{};
		α NewExpiration()Ι->steady_clock::time_point;
  };
	// α GetNewSessionId()ι->SessionPK;
	// α UpdateExpiration( SessionPK sessionId, const tcp::endpoint& UserEndpoint )ε->optional<Info>;
	// α Upsert( Info& info )ι->void;
	struct UpsertAwait{
		using Task = TTask<Info>; using Promise = Task::promise_type; using Handle = coroutine_handle<Promise>;
		UpsertAwait( str authorization, const tcp::endpoint& endpoint, bool socket, SRCE )ι:_authorization{authorization}, _endpoint{endpoint}, _socket{socket}, _sl{sl}{}
		α await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void;
		α await_resume()ε->Info;
	private:
		string _authorization; tcp::endpoint _endpoint; bool _socket; SL _sl; Promise* _promise;
	};
	//α Upsert( str sessionId, const tcp::endpoint& UserEndpoint, bool socket )ε->Info;
//TODO going off of old code
/*
	struct SessionInfoAwait{
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:_sessionId{sessionId}{}
		α await_ready()ι->bool;
		α await_suspend( HttpCo h )ι->void;
		α await_resume( HttpCo h )ι->net::awaitable<void, net::any_io_executor>;
	private:
		SessionPK _sessionId;
	};
	*/
}