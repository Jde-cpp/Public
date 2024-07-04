#include <jde/web/flex/Sessions.h>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/web/flex/HttpRequest.h>
//#include <jde/web/flex/RestException2.h>
#include "../../../../Framework/source/math/MathUtilities.h"

#define var const auto
namespace Jde::Web::Flex{
	using boost::concurrent_flat_map;
	concurrent_flat_map<SessionPK,Sessions::Info> _sessions;
	steady_clock::duration _restExpirationDuration{ Chrono::ToDuration(Settings::Get("http/timeout").value_or("PT30S")) };
	steady_clock::duration _sockExpirationDuration{ Chrono::ToDuration(Settings::Get("http/timeout").value_or("P1D")) };
	steady_clock::time_point _lastTrim{ steady_clock::now() };
	namespace Sessions{
		Info::Info( SessionPK sessionPK, const tcp::endpoint& userEndpoint, bool isSocket )ι:
			SessionId{ sessionPK },
			UserPK{},
			UserEndpoint{ userEndpoint },
			IsSocket{ isSocket },
			Expiration{ NewExpiration() },
			LastServerUpdate{steady_clock::now()},
			IsInitialRequest{true}{
			_sessions.emplace( sessionPK, *this );
		}
		α Info::NewExpiration()Ι->steady_clock::time_point{ return steady_clock::now()+(IsSocket ? _sockExpirationDuration : _restExpirationDuration); }
	}
	using namespace Sessions;
	α UpdateExpiration( SessionPK sessionId, const tcp::endpoint& userEndpoint )ε->optional<Info>{
		optional<Info> info;
		_sessions.visit( sessionId, [&info, &userEndpoint,sessionId]( auto& existing ){
			var& existingAddress = existing.second.UserEndpoint.address();
			THROW_IF( existingAddress!=userEndpoint.address(), "[{}]existingAddress='{}' does not match userEndpoint='{}'", sessionId, existingAddress.to_string(), userEndpoint.address().to_string() );
			auto& existingExpiration = existing.second.Expiration;
			if( existingExpiration>steady_clock::now() ){
				existingExpiration = existing.second.NewExpiration();
				info = existing.second;
			}
			else
				TRACET( WebTag(), "[{}]Session expired:  '{}'", sessionId, DateTime{existingExpiration}.ToIsoString() );
		} );
		if( _lastTrim<steady_clock::now()-_restExpirationDuration ){
			_sessions.erase_if( []( auto& x ){ return x.second.Expiration<steady_clock::now(); } );
			_lastTrim = steady_clock::now();
		}
		return info;
	}

	α Upsert( Info& info )ι->void{
		_sessions.emplace_or_visit( info.SessionId, info, [&info]( auto& existing ){ existing.second.Expiration=existing.second.NewExpiration(); } );
	}

	α GetNewSessionId()ι->SessionPK{
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}

	α Upsert( str authorization, tcp::endpoint endpoint, bool socket, UpsertAwait::Handle h )ε->Task{
		optional<Info>& info = h.promise().Result;
		if( !authorization.empty() ){
			optional<SessionPK> sessionId;
			try{
				if( sessionId = Str::TryTo<SessionPK>(string{authorization}, nullptr, 16);  !sessionId )
					THROW( "Could not create sessionId:  '{}'.", authorization );
				if( auto pInfo = UpdateExpiration(*sessionId, endpoint); pInfo )
					info = *pInfo;
				else{
					THROW_IF( Logging::Server::IsLocal(), "[{}]Session not found.", *sessionId );
					auto pServerInfo = awaitp( SessionInfo, Logging::Server::FetchSessionInfo(*sessionId, endpoint) ); THROW_IF( !pServerInfo, "[{}]AppServer did not have sessionInfo.", *sessionId );
					info = { *pServerInfo, endpoint, socket };
					Upsert( *info );
				}
			}
			catch( IException& e ){
				h.promise().Exception = e.Move();
			}
		}
		else{
			Info newSession{ GetNewSessionId(), endpoint, socket };//TODO create sessionId.
			Upsert( newSession );
			info = newSession;
		}
		h.resume();
	}

	α UpsertAwait::await_suspend( Handle h )ι->void{
		_promise = &h.promise();
		try{
			Upsert( _authorization, _endpoint, _socket, h );
		}
		catch( IException& e ){
			_promise->Exception = e.Move();
		}
	}
	α UpsertAwait::await_resume()ε->Info{
		if( _promise->Exception )
			_promise->Exception->Throw();
		ASSERT( _promise->Result );
		return *_promise->Result;
	}
}