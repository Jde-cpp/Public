#include <chrono>
#include <jde/web/server/Sessions.h>

#include <jde/web/server/HttpRequest.h>
#include <jde/fwk/str.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/web/server/auth/JwtLoginAwait.h>
#include <jde/app/IApp.h>
#include "ServerImpl.h"
#include "jde/fwk/exceptions/Exception.h"
#include "jde/fwk/usings.h"

#define let const auto
namespace Jde::Web::Server{
	constexpr ELogTags _tags{ ELogTags::Sessions };
	steady_clock::duration _restExpirationDuration{};
	α Sessions::RestSessionTimeout()ι->steady_clock::duration{
		if( _restExpirationDuration==steady_clock::duration::zero() )
			_restExpirationDuration = Chrono::ToDuration( Settings::FindSV("/http/timeout").value_or("PT30M") );
		return _restExpirationDuration;
	}
	steady_clock::duration _sockExpirationDuration{};
	Ω sockExpirationDuration(){
		if( _sockExpirationDuration==steady_clock::duration::zero() )
			_sockExpirationDuration = Chrono::ToDuration( Settings::FindSV("/http/socketTimeout").value_or("P1D") );
		return _sockExpirationDuration;
	}
	steady_clock::time_point _lastTrim{ steady_clock::now() };

	concurrent_flat_map<SessionPK,sp<SessionInfo>> _sessions;
	Ω upsert( sp<SessionInfo>& info )ι->void{
		if( _sessions.emplace_or_visit(info->SessionId, info, [hasSocket=info->HasSocket](auto& existing){
			existing.second->HasSocket|=hasSocket;
			existing.second->Expiration=existing.second->NewExpiration();
		}) ){
			TRACE( "Session added: id: {:x}, userPK: {}, endpoint: '{}', expiration: '{}'", info->SessionId, info->UserPK.Value, info->UserEndpoint, ToIsoString<seconds>(info->Expiration) );
		}
	}

	α GetNewSessionId()ι->SessionPK{
		auto sessionId{ Crypto::Random<uint32_t>() };
		while( _sessions.contains(sessionId) )
			sessionId = Crypto::Random<uint32_t>();
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
		upsert( newSession );
		return newSession;
	}

	α Sessions::Remove( SessionPK sessionId )ι->bool{
		return _sessions.erase( sessionId );
	}

	α Sessions::Find( SessionPK sessionId )ι->sp<SessionInfo>{
		sp<SessionInfo> y;
		_sessions.cvisit( sessionId, [&y](auto& kv){y = kv.second;} );
		return y;
	}

	α Sessions::Get()ι->vector<sp<SessionInfo>>{
		vector<sp<SessionInfo>> y;
		_sessions.cvisit_all( [&y](auto& kv){y.emplace_back(kv.second);} );
		return y;
	}
	α Sessions::Size()ι->uint{ return _sessions.size(); }

	SessionInfo::SessionInfo( SessionPK sessionPK, str userEndpoint, bool hasSocket )ι:
		SessionId{ sessionPK },
		UserEndpoint{ userEndpoint },
		HasSocket{ hasSocket },
		Expiration{ NewExpiration() },
		LastServerUpdate{ steady_clock::now() },
		IsInitialRequest{ true }
	{}

	SessionInfo::SessionInfo( SessionPK sessionPK, steady_clock::time_point expiration, Jde::UserPK userPK, str userEndpointAddress, bool hasSocket )ι:
		QL::IQLSession{ userPK },
		SessionId{ sessionPK },
		UserEndpoint{ userEndpointAddress },
		HasSocket{ hasSocket },
		Expiration{ expiration },
		LastServerUpdate{ steady_clock::now() }
	{}

	α SessionInfo::NewExpiration()Ι->steady_clock::time_point{
		return steady_clock::now()+( HasSocket ? sockExpirationDuration() : Sessions::RestSessionTimeout() );
	}

	Ω normalizedAddress( str s )ι->string{//::ffff:127.0.0.1 (an IPv4-mapped IPv6 address) and 127.0.0.1 name the same peer - strip the prefix so they compare equal.
		constexpr sv mapped{ "::ffff:" };
		return s.starts_with(mapped) && s.find('.')!=string::npos ? s.substr(mapped.size()) : s;
	}
	Ω sameEndpoint( str a, str b )ι->bool{//is this the address the session was minted for?
		let na = normalizedAddress( a ); let nb = normalizedAddress( b );
		if( na==nb )
			return true;
		boost::system::error_code eca, ecb;
		let aa = net::ip::make_address( na, eca ); let bb = net::ip::make_address( nb, ecb );
		return !eca && !ecb && aa.is_loopback() && bb.is_loopback();//127.0.0.1 vs ::1 - same host, so not a trust boundary; every other address requires an exact match.
	}
	α UpdateExpiration( SessionPK sessionId, str userEndpoint, bool socket )ε->sp<SessionInfo>{
		sp<SessionInfo> info;
		_sessions.visit( sessionId, [&info, sessionId, userEndpoint, socket](auto& kv){
			sp<SessionInfo> existing = kv.second;
			let& existingAddress = existing->UserEndpoint;
			if( !sameEndpoint(existingAddress, userEndpoint) ){//proof of possession - a session id is only a credential from the address it was minted for; otherwise a guessed/leaked id is a full takeover.  Return null so the caller reports the generic "Session not found", not an existence oracle for a bound-elsewhere id.
				WARNT( ELogTags::HttpServerRead, "[{:x}]Session endpoint '{}' does not match request endpoint '{}' - denying.", sessionId, existingAddress, userEndpoint );
				return;
			}
			auto& existingExpiration = existing->Expiration;
			if( existingExpiration>steady_clock::now() ){
				existing->HasSocket |= socket;//sticky - a socket connecting on a rest session promotes it to the socket timeout, later rest requests must not demote it.
				existingExpiration = existing->NewExpiration();
				info = existing;
			}
			else
				TRACET( ELogTags::HttpServerRead, "[{:x}]Session expired:  '{}'", sessionId, ToIsoString<seconds>(existingExpiration) );
		} );
		if( _lastTrim<steady_clock::now()-Sessions::RestSessionTimeout() ){
			_sessions.erase_if( [](auto& kv){return kv.second->Expiration<steady_clock::now();} );
			_lastTrim = steady_clock::now();
		}
		return info;
	}

namespace Sessions{
	α UpsertAwait::Suspend()ι->void{
		if( 	_authorization.starts_with("Bearer ") ){
			try{
				FromJwt( _authorization.substr(7) );
			}
			catch( runtime_error& e ){
				ResumeExp( move(e) );
			}
		}
		else if( _authorization.size() )
			FromSessionId();
		else
			CreateSession();
	}
	α UpsertAwait::FromJwt( str jwt )ι->TTask<UserPK>{
		try{
			auto userPK = co_await JwtLoginAwait{ Web::Jwt{jwt}, _endpoint, _appClient };
			CreateSession( userPK );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α UpsertAwait::CreateSession( UserPK userPK )ι->void{
		auto info = Sessions::Internal::CreateSession( userPK, _endpoint, _socket, false );
		upsert( info );
		Resume( move(info) );
	}
	α UpsertAwait::FromSessionId()ι->TTask<Web::FromServer::SessionInfo>{
		try{
			optional<SessionPK> sessionId = Str::TryTo<SessionPK>( string{_authorization}, nullptr, 16 );
			THROW_IF( !sessionId, "Invalid sessionId:  '{}'.", _authorization );
			auto info = UpdateExpiration( *sessionId, _endpoint, _socket );
			if( !info ){
				up<TAwait<Web::FromServer::SessionInfo>> await{ !_appClient || _appClient->IsLocal() ? nullptr : _appClient->SessionInfoAwait(*sessionId, _sl) };//3rd party, eg AppServer
				if( !await ){  //no 3rd party
					if( _throw )
						throw Exception( SRCE_CUR, ELogLevel::Debug, "[{}]Session not found.", Ƒ("{:x}", *sessionId) );
					else{
						_h.resume();
						co_return;
					}
				}
				try{
					Web::FromServer::SessionInfo proto{ co_await *await };
					let expiration = Chrono::ToClock<steady_clock,Clock>( Protobuf::ToTimePoint(proto.expiration()) );
					info = ms<SessionInfo>( *sessionId, expiration, UserPK{proto.user_pk()}, proto.user_endpoint(), proto.has_socket() );
					info->UserEndpoint = _endpoint;
					info->HasSocket = _socket;
				}
				catch( Exception& e ){
					//anonymous user,
					e.SetLevel(ELogLevel::Debug);
					info = ms<SessionInfo>( *sessionId, steady_clock::now()+RestSessionTimeout(), UserPK{0}, _endpoint, _socket );
				}
				upsert( info );
			}
			Resume( move(info) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α UpsertAwait::await_resume()ε->sp<SessionInfo>{
		base::CheckException();
		return Promise()->Value() ? *Promise()->Value() : sp<SessionInfo>{};
	}
}}