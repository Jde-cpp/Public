#include <jde/web/flex/Sessions.h>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/web/flex/HttpRequest.h>
#include "../../../../Framework/source/math/MathUtilities.h"

#define var const auto
namespace Jde::Web::Flex{
	using boost::concurrent_flat_map;
	namespace Sessions{
		steady_clock::duration _expirationDuration{ Chrono::ToDuration(Settings::Get("http/timeout").value_or("PT30S")) };//TODO get from config/server.
		steady_clock::time_point _lastTrim{ steady_clock::now() };//TODO get from config/server.
		concurrent_flat_map<SessionPK,Info> _sessions;

		Info::Info( SessionPK sessionPK, const tcp::endpoint& userEndpoint )ι:
			SessionId{sessionPK},
			UserPK{},
			UserEndpoint{userEndpoint},
			Expiration{steady_clock::now()+_expirationDuration},
			LastServerUpdate{steady_clock::now()},
			IsNew{true}{
			_sessions.emplace( sessionPK, *this );
		}
	}

	α Sessions::UpdateExpiration( SessionPK sessionId, const tcp::endpoint& userEndpoint )ε->optional<Info>{
		optional<Info> info;
		_sessions.visit( sessionId, [&info, &userEndpoint,sessionId]( auto& existing ){
			var& existingAddress = existing.second.UserEndpoint.address();
			THROW_IF( existingAddress!=userEndpoint.address(), "[{}]existingAddress='{}' does not match userEndpoint='{}'", sessionId, existingAddress.to_string(), userEndpoint.address().to_string() );
			auto& existingExpiration = existing.second.Expiration;
			if( existingExpiration>steady_clock::now() ){
				existingExpiration = steady_clock::now()+_expirationDuration;
				info = existing.second;
			}
			else
				TRACET( WebTag(), "[{}]Session expired:  '{}'", sessionId, DateTime{existingExpiration}.ToIsoString() );
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
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}
}