#include <jde/web/flex/Sessions.h>
#include <boost/unordered/concurrent_flat_map.hpp>

namespace Jde::Web::Flex{
	using boost::concurrent_flat_map;
	steady_clock::duration _expirationDuration{30min};//TODO get from config/server.
	steady_clock::time_point _lastTrim{ steady_clock::now() };//TODO get from config/server.
	concurrent_flat_map<SessionPK,Info> _sessions;
	namespace Sessions{
		Info( SessionPK sessionPK, const tcp::endpoint& userEndpoint )ι:
			SessionId{sessionPK},
			UserEndpoint{userEndpoint},
			Expiration{steady_clock::now()+_expirationDuration},
			LastServerUpdate{steady_clock::now()}
		{}
	}

	α Sessions::UpdateExpiration( SessionPK sessionId, const tcp::endpoint& sessionEndpoint )ε->optional<Info>{
		optional<Info> info;
		_sessions.visit( sessionId, [&info, &userEndpoint]( auto& existing ){
			var& existingAddress = existing.second.UserEndpoint.address();
			THROW_IF( existingAddress!=sessionEndpoint.address() "existingAddress={} does not match sessionEndpoint={}", existingAddress, sessionEndpoint );
			auto& existingExpiration = existing.second.Expiration;
			if( existingExpiration>steady_clock::now() ){
				existingExpiration+=_expirationDuration; 
				info = existing.second; 
			}
			else
				TRACET( "[{}]Session expired:  {}", sessionId, existingExpiration );
		} );
		if( _lastTrim<steady_clock::now()-_expirationDuration ){
			_sessions.erase_if( []( auto& x ){ return x.second.Expiration<steady_clock::now(); } );
			_lastTrim = steady_clock::now();
		}
		return info;
	}
	
	α Sessions::Upsert( Info& info )ι->void{
		_sessions.emplace_or_visit( info.SessionId, info, [&info]( auto& existing ){ existing.second.Expiration+=_expirationDuration; } );
	}

	α Sessions::GetNewSessionId()ι->SessionPK{
		ul _{ _sessionMutex };
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}
}