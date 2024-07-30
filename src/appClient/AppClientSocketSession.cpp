#include <jde/appClient/AppClientSocketSession.h>
#include "../../../Framework/source/io/AsioContextThread.h"
#include <jde/web/flex/Flex.h>
#include <jde/appClient/proto/App.FromClient.h>
#include <jde/appClient/proto/Common.h>
#include <jde/web/client/ClientSocketAwait.h>
#include <jde/appClient/StringCache.h>

#define var const auto

namespace Jde::App{
	using Http::ClientSocketAwait; using IO::Proto::ToString;

	sp<Client::AppClientSocketSession> _pSession;
	α Client::CloseSocketSession( SL sl )ι->VoidTask{
		if( _pSession ){
			var tags = ELogTags::Client | ELogTags::Socket;
			Trace( sl, tags, "ClosingSocketSession" );
			co_await _pSession->Close();
			_pSession = nullptr;
			Information( sl, tags, "ClosedSocketSession" );
		}
	}
	α Client::GraphQL( str query, SL sl )ε->Http::ClientSocketAwait<string>{
		auto p = _pSession; THROW_IF( !p, "Not connected." );
		auto requestId = p->NextRequestId();
		return Http::ClientSocketAwait<string>{ ToString(FromClient::GraphQL(query, requestId)), requestId, p, sl };
	}
namespace Client{

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
/*	α AppClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<SessionPK>{
		var requestId = NextRequestId();
		return ClientSocketAwait<SessionPK>{ shared_from_this(), requestId, ToString(FromClient::ConnectTransmission(sessionId, requestId)), sl };
	}*/

	α AppClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void {
			HandleException(move(h), CodeException{ec, Http::SocketClientReadTag(), ELogLevel::NoLog}, false );
		};
		CloseTasks( f );
		//TODO! - restart
		base::OnClose( ec );
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::SessionInfo>{
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::FromServer::SessionInfo>{ ToString(FromClient::Session(sessionId, requestId)), requestId, shared_from_this(), sl };
	}

	template<class T,class... Args> auto Resume( std::any&& h, T& v, fmt::format_string<Args...> m="", Args&&... args )ι->void{
		auto p = std::any_cast<typename ClientSocketAwait<T>::Handle>( &h );
		p->promise().Log( m, std::forward<Args>(args)... );
		p->promise().SetValue( std::forward<T>(move(v)) );
		p->resume();
	}
	template<class T,class... Args> auto ResumeScaler( std::any&& h, T v, fmt::format_string<Args...> m="", Args&&... args )ι->void{
		Resume( move(h), v, m, std::forward<Args>(args)... );
	}

	α AppClientSocketSession::Execute( string&& bytes, optional<UserPK> userPK, RequestId clientRequestId )ι->void{
		try{
			auto t = IO::Proto::Deserialize<Proto::FromServer::Transmission>( move(bytes) );
			ProcessTransmission( move(t), userPK, clientRequestId );
		}
		catch( IException& e ){
			WriteException( move(e), clientRequestId );
		}
	}

	α AppClientSocketSession::OnRead( Proto::FromServer::Transmission&& t )ι->void{
		ProcessTransmission( move(t), _userPK, nullopt );
	}

	α AppClientSocketSession::ProcessTransmission( Proto::FromServer::Transmission&& t, optional<UserPK> userPK, optional<RequestId> clientRequestId )ι->void{
		for( auto i=0; i<t.messages_size(); ++i ){
			auto m = t.mutable_messages( i );
			using enum Proto::FromServer::Message::ValueCase;
			var requestId = clientRequestId.value_or( m->request_id() );
			std::any hAny = requestId ? IClientSocketSession::GetTask( requestId ) : nullptr;
			switch( m->Value_case() ){
			[[unlikely]] case kAck:{
				var serverSocketId = m->ack();
				SetId( serverSocketId );
				INFOT( Http::SocketClientReadTag(), "[{}] {} AppClientSocketSession created: {}.", Id(), IsSsl() ? "Ssl" : "Plain", Host() );
				ResumeScaler<SessionPK>( move(hAny), SessionId() );
				}break;
			case kGeneric:
				Resume( move(hAny), *m->mutable_generic(), "Generic - '{}'.", m->generic() );
				break;
			[[likely]] case kStrings:{
				auto& res = *m->mutable_strings();
				Resume( move(hAny), *m->mutable_strings(), "Strings count='{}'.", res.messages().size()+res.files().size()+res.functions().size()+res.threads().size() );
				}break;
			case kLogLevels:{//TODO implement when have tags.
				auto& res = *m->mutable_log_levels();
				Resume( move(hAny), res, "LogLevel server='{}', client='{}'.", ToString((ELogLevel)res.server()), ToString((ELogLevel)res.client()) );
				}break;
			case kProgress://TODO not awaitable
				ResumeScaler( move(hAny), m->progress(), "Progress: '{}'.", m->progress() );
				break;
			case kSessionInfo:{
				auto& res = *m->mutable_session_info();
				Resume( move(hAny), res, "SessionInfo: expiration: '{}', session_id: '{}', user_pk: '{}', user_endpoint: '{}'.", ToIsoString(IO::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				}break;
			case kGraphQl:
				Resume( move(hAny), *m->mutable_graph_ql(), "GraphQl: '{}'.", m->graph_ql().substr(0, Web::MaxLogLength()) );
				break;
			case kException:
				HandleException( move(hAny), Jde::Proto::ToException(move(*m->mutable_exception())), requestId );
				break;
			case kExecute:
			case kExecuteAnonymous:{
				bool isAnonymous = m->Value_case()==kExecuteAnonymous;
				auto bytes = isAnonymous ? move( *m->mutable_execute_anonymous() ) : move( *m->mutable_execute()->mutable_transmission() );
				optional<UserPK> userPK = m->Value_case()==kExecuteAnonymous ? nullopt : optional<UserPK>(m->execute().user_pk() );
				LogRead( "Execute{} size: {:10L}", isAnonymous ? "Anonymous" : "", bytes.size()  );
				Execute( move(bytes), userPK, requestId );
				break;}
			case kExecuteResponse://wait for use case.
			case kStringPks://strings already saved in db, no need to send.  not being requested by client yet.
				CRITICALT( Http::SocketClientReadTag(), "[{:x}]No use case has been implemented on client app '{}'.", Id(), underlying(m->Value_case()) );
			case kTraces:
			case kStatus:
				CRITICALT( Http::SocketClientReadTag(), "[{:x}]Web only call not implemented on client app '{}'.", Id(), (uint)m->Value_case() );
			break;
			case VALUE_NOT_SET:
				break;
			}
		}
	}
	α AppClientSocketSession::HandleException( std::any&& h, IException&& e, RequestId requestId )ι->void{
		auto handle = [&]( sv msg, auto pAwait ){
			pAwait->promise().ResponseMessage = "Error: {}";
			pAwait->promise().MessageArgs.emplace_back( e.what() );
			pAwait->promise().SetError( move(e) );
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
			ELogLevel severity = requestId ? ELogLevel::Critical : ELogLevel::Debug;
			Logging::Log( Logging::Message(severity, "[0]Failed to process incoming exception '{}'."), Http::SocketClientReadTag(), requestId, e.what() );
		}
	}
	α AppClientSocketSession::WriteException( IException&& e, RequestId requestId )->void{
		Write( FromClient::Exception(move(e)) );
	}
}}