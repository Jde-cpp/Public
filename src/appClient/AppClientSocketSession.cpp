#include "AppClientSocketSession.h"
#include <jde/web/flex/Flex.h>
#include <jde/appClient/proto/App.FromClient.h>
#include "../../../Framework/source/io/AsioContextThread.h"
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
	using Http::ClientSocketAwait; using IO::Proto::ToString;

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
		auto f = [this, ec](std::any&& h)->void { HandleException(move(h), CodeException{ec, Http::SocketClientReceivedTag(), ELogLevel::NoLog }.what(), false ); };
		CloseTasks( f );
		//TODO - restart
		base::OnClose( ec );
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::SessionInfo>{
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::FromServer::SessionInfo>{ shared_from_this(), requestId, ToString(FromClient::SessionInfoMessage(sessionId, requestId)), sl };
	}
	template<class T,class... Args> auto Resume( std::any&& h, T&& v, fmt::format_string<Args...> m="", Args&&... args )ι->void{
		auto p = std::any_cast<typename ClientSocketAwait<T>::Handle>( &h );
		p->promise().Log( m, std::forward<Args>(args)... );
		p->promise().SetValue( std::forward<T>(v) );
		p->resume();
	}
	α AppClientSocketSession::OnRead( Proto::FromServer::Transmission&& t )ι->void{
/*		auto Resume = []<class T>( std::any&& hAny, T&& v ){
			auto h = std::any_cast<typename ClientSocketAwait<T>::Handle>( &hAny );
			h->promise().Result.emplace( std::forward<T>(v) );
			h->promise().Log( "{}", "test" );
			h->resume();
		};*/
		for( auto i=0; i<t.messages_size(); ++i ){
			auto m = t.mutable_messages( i );
			using enum Proto::FromServer::Message::ValueCase;
			var requestId = m->request_id();
			//auto _logTag = SocketClientReceivedTag();
			std::any hAny = requestId ? IClientSocketSession::GetTask( requestId ) : nullptr;
			switch( m->Value_case() ){
			[[unlikely]] case kAck:{
				var ack = m->ack();
				SetSessionId( ack.session_id() );
				SetId( ack.server_socket_id() );
				INFOT( Http::SocketClientReceivedTag(), "{} AppClientSocketSession created: {}.", Id(), IsSsl() ? "Ssl" : "Plain", Host() );
				Resume<SessionPK>( move(hAny), SessionId() );
				// auto h = std::any_cast<ClientSocketAwait<uint32>::Handle>( &hAny );
				// h->promise().Result = move( sessionId );
				// h->resume(); //TODO one function
				}break;
			case kGeneric:
				//auto log = []( auto start ){ DBG( "Generic duration: {}ms", duration_cast<milliseconds>(steady_clock::now()-start).count() ); };
				Resume( move(hAny), move(*m->mutable_generic()), "Generic - '{}'.", m->generic().substr(0, Web::MaxLogLength()) );
				// auto h = std::any_cast<ClientSocketAwait<string>::Handle>( &hAny );
				// h->promise().Result = move( *m->mutable_generic() );
				// h->resume();
				break;
			[[likely]] case kStrings:{
				auto& res = *m->mutable_strings();
				Resume( move(hAny), move(*m->mutable_strings()), "Strings count='{}'.", res.messages().size()+res.files().size()+res.functions().size()+res.threads().size() );
				}break;
			case kLogLevels:{//TODO not awaitable
				auto& res = *m->mutable_log_levels();
				Resume( move(hAny), move(res), "LogLevel server='{}', client='{}'.", ToString((ELogLevel)res.server()), ToString((ELogLevel)res.client()) );
				}break;
			case kCustom://TODO not awaitable
				Resume( move(hAny), move(*m->mutable_custom()), "Custom size='{}'.", m->custom().size() );
				break;
			case kProgress://TODO not awaitable
				Resume( move(hAny), m->progress(), "Progress: '{}'.", m->progress() );
				break;
			case kSessionInfo:{//TODO not awaitable
				auto& res = *m->mutable_session_info();
				Resume( move(hAny), move(res), "SessionInfo: expiration: '{}', session_id: '{}', user_pk: '{}', user_endpoint: '{}'.", ToIsoString(IO::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				}break;
			case kAddSessionResult:
				Resume( move(hAny), m->add_session_result(), "AddSessionResult: '{}'.", m->add_session_result() );
				break;
			case kGraphQl:
				Resume( move(hAny), move(*m->mutable_graph_ql()), "GraphQl: '{}'.", m->graph_ql().substr(0, Web::MaxLogLength()) );
				break;
			case kException:
				HandleException( move(hAny), move(*m->mutable_exception()), requestId==0 );
				break;
			case kTraces:
			case kApplications:
			case kStatus:
				CRITICALT( Http::SocketClientReceivedTag(), "[{:x}]Web only call not implemented on client app '{}'.", Id(), (uint)m->Value_case() );
			break;
			case VALUE_NOT_SET:
				break;
			}
		}
	}
	α AppClientSocketSession::HandleException( std::any&& h, string&& what, bool fromRequest )ι->void{
		auto handle = [&]( sv msg, auto pAwait ){
			pAwait->promise().ResponseMessage = msg;
			pAwait->promise().MessageArgs.emplace_back( what );
			pAwait->promise().SetError( mu<Exception>(move(what)) );
			pAwait->resume();
		};
		if( auto pAwait = std::any_cast<ClientSocketAwait<uint32>::Handle>(&h) )
			handle( "Exception<uint32>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<string>::Handle>(&h) )
			handle( "Exception<string>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Proto::FromServer::Strings>::Handle>(&h) )
			handle( "Exception<Strings>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Proto::FromServer::SessionInfo>::Handle>(&h) )
			handle( "Exception<SessionInfo>: '{}'.", pAwait );
		else{
			ELogLevel severity = fromRequest ? ELogLevel::Debug : ELogLevel::Critical;
			Logging::Log( Logging::Message(severity, "Failed to process incoming exception '{}'."), Http::SocketClientReceivedTag(), what );
		}
	}
}}