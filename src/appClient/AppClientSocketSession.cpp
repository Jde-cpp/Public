#include "AppClientSocketSession.h"
#include "../../../Framework/source/io/AsioContextThread.h"
#include "proto/App.FromClient.h"
//#include <jde/http/ClientSocketAwait.h>
#define var const auto

namespace Jde::App{
	sp<Client::AppClientSocketSession> _pSession;
	α Client::CloseSocketSession()ι->VoidTask{
		if( _pSession ){
			co_await _pSession->Close();
			_pSession = nullptr;
		}
	}
namespace Client{
	using Http::ClientSocketAwait; using Http::IncomingTag; using IO::Proto::ToString;

	α StartSocketAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		ASSERT( IO::AsioContextThread() );
		_pSession = ms<Client::AppClientSocketSession>( IO::AsioContextThread(), _isSsl ? ssl::context(ssl::context::tlsv12_client) : optional<ssl::context>{} );
		[this,h]()->VoidTask { co_await _pSession->RunSession( _host, _port ); h.resume(); }();
	}

	α AppClientSocketSession::Instance()ι->sp<AppClientSocketSession>{ return _pSession; }
	AppClientSocketSession::AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι:
		base( ioc, ctx )
	{}
	α AppClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<SessionPK>{
		var requestId = NextRequestId();
		return ClientSocketAwait<SessionPK>{ shared_from_this(), requestId, ToString(FromClient::ConnectMessage(sessionId, requestId)), sl };
	}

	α AppClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void { HandleException(move(h), CodeException{ec, IncomingTag()}.what() ); };
		CloseTasks( f );
		//TODO - restart
		base::OnClose( ec );
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::SessionInfo>{
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::FromServer::SessionInfo>{ shared_from_this(), requestId, ToString(FromClient::SessionInfoMessage(sessionId, requestId)), sl };
	}

	α AppClientSocketSession::OnRead( Proto::FromServer::Transmission&& t )ι->void{
		auto Resume = []<class T>( std::any&& hAny, T&& v ){
			auto h = std::any_cast<typename ClientSocketAwait<T>::Handle>( &hAny );
			h->promise().Result.emplace( std::forward<T>(v) );
			h->resume();
		};
		for( auto i=0; i<t.messages_size(); ++i ){
			auto m = t.mutable_messages( i );
			using enum Proto::FromServer::Message::ValueCase;
			var requestId = m->request_id();
			auto _logTag = IncomingTag();
			std::any hAny = requestId ? IClientSocketSession::GetTask( requestId ) : nullptr;
			switch( m->Value_case() ){
			[[unlikely]] case kAck:{
				var sessionId = m->ack();
				SetSessionId( sessionId );
				INFO( "[{:x}]{} AppClientSocketSession created: {}.", sessionId, IsSsl() ? "Ssl" : "Plain", Host() );
				Resume( move(hAny), move(sessionId) );
				// auto h = std::any_cast<ClientSocketAwait<uint32>::Handle>( &hAny );
				// h->promise().Result = move( sessionId );
				// h->resume(); //TODO one function
				}break;
			case kGeneric:
				TRACE( "[{:x}]Generic - '{}'.", SessionId(), m->generic().substr(0, MaxLogLength()) );
				Resume( move(hAny), move(*m->mutable_generic()) );
				// auto h = std::any_cast<ClientSocketAwait<string>::Handle>( &hAny );
				// h->promise().Result = move( *m->mutable_generic() );
				// h->resume();
				break;
			[[likely]] case kStrings:{
				auto& res = *m->mutable_strings();
				TRACE( "[{:x}]Strings count='{}'.", SessionId(), res.messages().size()+res.files().size()+res.functions().size()+res.threads().size() );
				Resume( move(hAny), move(*m->mutable_strings()) );
				}break;
			case kLogLevels:{//TODO not awaitable
				auto& res = *m->mutable_log_levels();
				TRACE( "[{:x}]LogLevel server='{}', client='{}'.", SessionId(), ToString((ELogLevel)res.server()), ToString((ELogLevel)res.client()) );
				Resume( move(hAny), move(res) );
				}break;
			case kCustom://TODO not awaitable
				TRACE( "[{:x}]Custom size='{}'.", SessionId(), m->custom().size() );
				Resume( move(hAny), move(*m->mutable_custom()) );
				break;
			case kProgress://TODO not awaitable
				TRACE( "[{:x}]Progress: '{}'.", SessionId(), m->progress() );
				Resume( move(hAny), m->progress() );
				break;
			case kSessionInfo:{//TODO not awaitable
				auto& res = *m->mutable_session_info();
				TRACE( "[{:x}]SessionInfo: expiration: '{}', session_id: '{}', user_pk: '{}', user_endpoint: '{}'.", ToIsoString(IO::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				Resume( move(hAny), move(res) );
				}break;
			case kAddSessionResult:
				TRACE( "[{:x}]AddSessionResult: '{}'.", SessionId(), m->add_session_result() );
				Resume( move(hAny), m->add_session_result() );
				break;
			case kGraphQl:
				TRACE( "[{:x}]GraphQl: '{}'.", SessionId(), m->graph_ql().substr(0, MaxLogLength()) );
				Resume( move(hAny), move(*m->mutable_graph_ql()) );
				break;
			case kException:
				HandleException( move(hAny), move(*m->mutable_exception()) );
				break;
			case VALUE_NOT_SET:
				break;
			}
		}
	}
	α AppClientSocketSession::HandleException( std::any&& h, string&& what )ι->void{
		// if( auto pEcho = std::any_cast<ClientSocketAwait<string>::Handle>( &h ) ){
		// 	pEcho->promise().Exception = mu<Exception>( what );
		// 	pEcho->resume();
		// }
		auto _logTag = IncomingTag();
		auto pException = mu<Exception>( move(what) );
		auto log = sv{ pException->What().substr(0, MaxLogLength()) };
		if( auto pAwait = std::any_cast<ClientSocketAwait<uint32>::Handle>(&h) ){
			DBG( "[{:x}]Exception<uint32>: '{}'.", SessionId(), log );
			pAwait->promise().Exception = move( pException );
			pAwait->resume();
		}
		else if( auto pAwait = std::any_cast<ClientSocketAwait<string>::Handle>(&h) ){
			DBG( "[{:x}]Exception<string>: '{}'.", SessionId(), log );
			pAwait->promise().Exception = move( pException );
			pAwait->resume();
		}
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Proto::FromServer::Strings>::Handle>(&h) ){
			DBG( "[{:x}]Exception<Strings>: '{}'.", SessionId(), log );
			pAwait->promise().Exception = move( pException );
			pAwait->resume();
		}
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Proto::FromServer::SessionInfo>::Handle>(&h) ){
			DBG( "[{:x}]Exception<SessionInfo>: '{}'.", SessionId(), log );
			pAwait->promise().Exception = move( pException );
			pAwait->resume();
		}
		else{
			CRITICAL( "Failed to process incomming exception '{}'.", what );
		}
	}
}}