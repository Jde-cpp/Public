#include <jde/appClient/Sessions.h>
#include <jde/web/flex/HttpRequest.h>
#include <jde/appClient/usings.h>
#include "../../../Framework/source/math/MathUtilities.h"
#include <jde/appClient/AppClient.h>

#define var const auto
namespace Jde{
	namespace Web{ concurrent_flat_map<SessionPK,SessionInfo> _sessions; }

	α Web::FindSession( SessionPK sessionId )ι->optional<SessionInfo>{
		optional<SessionInfo> y;
		_sessions.cvisit( sessionId, [&y]( auto& kv ){ y = kv.second; } );
		return y;
	}
	α Web::GetSessions()ι->vector<SessionInfo>{
		vector<SessionInfo> y;
		_sessions.cvisit_all( [&y]( auto& kv ){ y.emplace_back(kv.second); } );
		return y;
	}
	α Web::SessionSize()ι->uint{ return _sessions.size(); }
namespace Web{
	steady_clock::duration _restExpirationDuration{ Chrono::ToDuration(Settings::Get("http/timeout").value_or("PT30S")) };
	steady_clock::duration _sockExpirationDuration{ Chrono::ToDuration(Settings::Get("http/timeout").value_or("P1D")) };
	steady_clock::time_point _lastTrim{ steady_clock::now() };

	SessionInfo::SessionInfo( SessionPK sessionPK, str userEndpoint, bool hasSocket )ι:
		SessionId{ sessionPK },
		UserPK{},
		UserEndpoint{ userEndpoint },
		HasSocket{ hasSocket },
		Expiration{ NewExpiration() },
		LastServerUpdate{ steady_clock::now() },
		IsInitialRequest{ true }{
		_sessions.emplace( sessionPK, *this );
	}
	SessionInfo::SessionInfo( SessionPK sessionPK, steady_clock::time_point expiration, Jde::UserPK userPK, str userEndpointAddress, bool hasSocket )ι:
		SessionId{ sessionPK },
		UserPK{ userPK },
		UserEndpoint{ userEndpointAddress },
		HasSocket{ hasSocket },
		Expiration{ expiration },
		LastServerUpdate{ steady_clock::now() }
	{}

	α SessionInfo::NewExpiration()Ι->steady_clock::time_point{ return steady_clock::now()+(HasSocket ? _sockExpirationDuration : _restExpirationDuration); }

	α UpdateExpiration( SessionPK sessionId, str userEndpoint )ε->optional<SessionInfo>{
		optional<SessionInfo> info;
		_sessions.visit( sessionId, [&info, &userEndpoint, sessionId]( auto& existing ){
			var& existingAddress = existing.second.UserEndpoint;
			THROW_IF( existingAddress!=userEndpoint, "[{}]existingAddress='{}' does not match userEndpoint='{}'", sessionId, existingAddress, userEndpoint );
			auto& existingExpiration = existing.second.Expiration;
			if( existingExpiration>steady_clock::now() ){
				existingExpiration = existing.second.NewExpiration();
				info = existing.second;
			}
			else
				TRACET( Web::Flex::WebTag(), "[{}]Session expired:  '{}'", sessionId, DateTime{existingExpiration}.ToIsoString() );
		} );
		if( _lastTrim<steady_clock::now()-_restExpirationDuration ){
			_sessions.erase_if( []( auto& x ){ return x.second.Expiration<steady_clock::now(); } );
			_lastTrim = steady_clock::now();
		}
		return info;
	}

	α Upsert( SessionInfo& info )ι->void{
		_sessions.emplace_or_visit( info.SessionId, info, [&info]( auto& existing ){ existing.second.Expiration=existing.second.NewExpiration(); } );
	}

	α GetNewSessionId()ι->SessionPK{
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}

	α Upsert( str authorization, str endpoint, bool socket, UpsertAwait::Handle h )ε->TTask<SessionInfo>{
		optional<SessionInfo> info;
		if( !authorization.empty() ){
			optional<SessionPK> sessionId;
			try{
				if( sessionId = Str::TryTo<SessionPK>(string{authorization}, nullptr, 16);  !sessionId )
					THROW( "Could not create sessionId:  '{}'.", authorization );
				if( auto pInfo = UpdateExpiration(*sessionId, endpoint); pInfo )
					info = move( *pInfo );
				else{
					THROW_IF( App::IsAppServer(), "[{}]Session not found.", *sessionId );
					info = co_await App::Client::SessionInfoAwait{ *sessionId }; //TODO test case
					info->UserEndpoint = endpoint;
					info->HasSocket = socket;
					Upsert( *info );
				}
			}
			catch( IException& e ){
				h.promise().SetError( e.Move() );
			}
		}
		else{
			SessionInfo newSession{ GetNewSessionId(), endpoint, socket };//TODO create sessionId.
			Upsert( newSession );
			info = move( newSession );
		}
		if( info )
			h.promise().SetValue( move(*info) );
		h.resume();
	}

	α UpsertAwait::await_suspend( Handle h )ι->void{
		base::await_suspend( h );
		try{
			Upsert( _authorization, _endpoint, _socket, h );
		}
		catch( IException& e ){
			Promise()->SetError( e.Move() );
		}
	}
	α UpsertAwait::await_resume()ε->SessionInfo{
		base::AwaitResume();
		ASSERT( Promise()->Value() );
		return move( *Promise()->Value() );
	}
}}
