#pragma once
//#include "HttpRequestAwait.h"
#include "../usings.h"

namespace Jde::Web::Flex::Sessions{

	struct Info{
		Info()ι=default;
		Info( SessionPK sessionPK, const tcp::endpoint& UserEndpoint )ι;
		Info( const SessionInfo& proto, const tcp::endpoint& userEndpoint )ι:SessionId{proto.session_id()}, UserPK{proto.user_id()}, UserEndpoint{userEndpoint}{}
//		α operator==(const Info& b)const{ return SessionId==b.SessionId && UserEndpoint==b.UserEndpoint; }
//		struct HashFunction{ α operator()(const Info& x)const{ return std::hash<SessionPK>()(x.SessionId) ^ std::hash<tcp::endpoint>()(x.UserEndpoint); } };

		SessionPK SessionId;
		Jde::UserPK UserPK;
		tcp::endpoint UserEndpoint;
		steady_clock::time_point Expiration;
		steady_clock::time_point LastServerUpdate;
		bool IsNew{};
  };
	α GetNewSessionId()ι->SessionPK;
	α UpdateExpiration( SessionPK sessionId, const tcp::endpoint& UserEndpoint )ε->optional<Info>;
	α Upsert( Info& info )ι->void;
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