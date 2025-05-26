#include <jde/web/server/Sessions.h>

#include <jde/web/server/HttpRequest.h>
#include <jde/framework/str.h>
#include "../../../../Framework/source/math/MathUtilities.h"
#include "ServerImpl.h"

#define let const auto
namespace Jde::Web::Server{
	constexpr ELogTags _tags{ ELogTags::Sessions };
	steady_clock::duration _restExpirationDuration{};
	α Sessions::RestSessionTimeout()ι->steady_clock::duration{ 
		if( _restExpirationDuration==steady_clock::duration::zero() )
			_restExpirationDuration = Chrono::ToDuration( Settings::FindSV("/http/timeout").value_or("PT30S") );
		return _restExpirationDuration; 
	}
	steady_clock::duration _sockExpirationDuration{};
	Ω sockExpirationDuration(){
		if( _sockExpirationDuration==steady_clock::duration::zero() )
			_sockExpirationDuration = Chrono::ToDuration( Settings::FindSV("/http/timeout").value_or("P1D") );
		return _sockExpirationDuration;
	}
	steady_clock::time_point _lastTrim{ steady_clock::now() };

	concurrent_flat_map<SessionPK,sp<SessionInfo>> _sessions;
	α Upsert( sp<SessionInfo>& info )ι->void{
		if( _sessions.emplace_or_visit(info->SessionId, info, [&info]( auto& existing ){ existing.second->Expiration=existing.second->NewExpiration();}) )
			Trace{ _tags, "Session added: id: {:x}, userPK: {}, endpoint: '{}'", info->SessionId, info->UserPK.Value, info->UserEndpoint };
	}

	α GetNewSessionId()ι->SessionPK{
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}

namespace	Sessions{
	α Internal::CreateSession( UserPK userPK, str endpoint, bool isSocket, bool add )ι->sp<SessionInfo>{
		auto y = sp<SessionInfo>( new SessionInfo{GetNewSessionId(), endpoint, isSocket} );
		y->UserPK = userPK;
		if( add )
			_sessions.emplace( y->SessionId, y );
		return y;
	}
}
	α Sessions::Add( UserPK userPK, string&& endpoint, bool isSocket )ι->sp<SessionInfo>{
		auto newSession = Internal::CreateSession( userPK, move(endpoint), isSocket, false );
		Upsert( newSession );
		return newSession;
	}

	α Sessions::Remove( SessionPK sessionId )ι->bool{
		return _sessions.erase( sessionId );
	}

	α Sessions::Find( SessionPK sessionId )ι->sp<SessionInfo>{
		sp<SessionInfo> y;
		_sessions.cvisit( sessionId, [&y](auto& kv){ y = kv.second; } );
		return y;
	}

	α Sessions::Get()ι->vector<sp<SessionInfo>>{
		vector<sp<SessionInfo>> y;
		_sessions.cvisit_all( [&y]( auto& kv ){ y.emplace_back(kv.second); } );
		return y;
	}
	α Sessions::Size()ι->uint{ return _sessions.size(); }

	SessionInfo::SessionInfo( SessionPK sessionPK, str userEndpoint, bool hasSocket )ι:
		SessionId{ sessionPK },
		UserPK{},
		UserEndpoint{ userEndpoint },
		HasSocket{ hasSocket },
		Expiration{ NewExpiration() },
		LastServerUpdate{ steady_clock::now() },
		IsInitialRequest{ true }
	{}

	SessionInfo::SessionInfo( SessionPK sessionPK, steady_clock::time_point expiration, Jde::UserPK userPK, str userEndpointAddress, bool hasSocket )ι:
		SessionId{ sessionPK },
		UserPK{ userPK },
		UserEndpoint{ userEndpointAddress },
		HasSocket{ hasSocket },
		Expiration{ expiration },
		LastServerUpdate{ steady_clock::now() }
	{}

	α SessionInfo::NewExpiration()Ι->steady_clock::time_point{
		return steady_clock::now()+( HasSocket ? sockExpirationDuration() : Sessions::RestSessionTimeout() );
	}

	α UpdateExpiration( SessionPK sessionId, str userEndpoint )ε->sp<SessionInfo>{
		sp<SessionInfo> info;
		_sessions.visit( sessionId, [&info, &userEndpoint, sessionId]( auto& kv ){
			sp<SessionInfo> existing = kv.second;
			let& existingAddress = existing->UserEndpoint;
			//THROW_IF( existingAddress!=userEndpoint, "[{}]existingAddress='{}' does not match userEndpoint='{}'", sessionId, existingAddress, userEndpoint );
			auto& existingExpiration = existing->Expiration;
			if( existingExpiration>steady_clock::now() ){
				existingExpiration = existing->NewExpiration();
				info = existing;
			}
			else
				Trace{ ELogTags::HttpServerRead, "[{:x}]Session expired:  '{}'", sessionId, DateTime{existingExpiration}.ToIsoString() };
		} );
		if( _lastTrim<steady_clock::now()-Sessions::RestSessionTimeout() ){
			_sessions.erase_if( []( auto& kv ){ return kv.second->Expiration<steady_clock::now(); } );
			_lastTrim = steady_clock::now();
		}
		return info;
	}

namespace Sessions{
	α UpsertAwait::Execute()ι->TTask<Web::FromServer::SessionInfo>{
		sp<SessionInfo> info;
		if( !_authorization.empty() ){
			optional<SessionPK> sessionId;
			try{
				if( sessionId = Str::TryTo<SessionPK>(string{_authorization}, nullptr, 16);  !sessionId )
					THROW( "Invalid sessionId:  '{}'.", _authorization );
				if( auto pInfo = UpdateExpiration(*sessionId, _endpoint); pInfo )
					info = pInfo;
				else{
					up<TAwait<Web::FromServer::SessionInfo>> pAwait = Server::SessionInfoAwait( *sessionId ); //3rd party, eg AppServer
					if( !pAwait ){  //no 3rd party
						if( _throw )
							throw Exception( SRCE_CUR, ELogLevel::Debug, "[{}]Session not found.", Ƒ("{:x}", *sessionId) );
						else{
							_h.resume();
							co_return;
						}
					}
					Web::FromServer::SessionInfo proto = co_await *pAwait;
					steady_clock::time_point expiration = Chrono::ToClock<steady_clock,Clock>( Proto::ToTimePoint(proto.expiration()) );
					info = ms<SessionInfo>( *sessionId, expiration, UserPK{proto.user_pk()}, proto.user_endpoint(), proto.has_socket() );
					pAwait.reset();
					info->UserEndpoint = _endpoint;
					info->HasSocket = _socket;
					Upsert( info );
				}
			}
			catch( IException& e ){
				Promise()->SetExp( move(e) );
			}
		}
		else{
			auto newSession = Sessions::Internal::CreateSession( {}, _endpoint, _socket, false );
			Upsert( newSession );
			info = newSession;
		}
		if( info )
			Promise()->SetValue( move(info) );
		_h.resume();
	}

	α UpsertAwait::Suspend()ι->void{
		try{
			Execute();
		}
		catch( IException& e ){
			Promise()->SetExp( move(e) );
		}
	}
	α UpsertAwait::await_resume()ε->sp<SessionInfo>{
		base::AwaitResume();
		return Promise()->Value() ? *Promise()->Value() : sp<SessionInfo>{};
	}
}}