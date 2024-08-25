#include <jde/app/client/AppClientSocketSession.h>
#include <jde/thread/Execution.h>
//#include <jde/web/server/Flex.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/app/shared/proto/Common.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/app/client/AppClient.h>
#include <jde/app/shared/StringCache.h>

#define var const auto

namespace Jde::App{
	using Web::Client::ClientSocketAwait; using IO::Proto::ToString;
	constexpr ELogTags _tags{ ELogTags::SocketClientRead };

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
	α Client::AddSession( str domain, str loginName, ProviderPK providerPK, str userEndPoint, bool isSocket, SL sl )ι->Web::Client::ClientSocketAwait<Proto::FromServer::SessionInfo>{
		auto p = _pSession; THROW_IF( !p, "Not connected." );
		auto requestId = p->NextRequestId();
		Trace( sl, ELogTags::SocketClientWrite, "AddSession domain: '{}', loginName: '{}', providerPK: {}, userEndPoint: '{}', isSocket: {}.", domain, loginName, providerPK, userEndPoint, isSocket );
		return ClientSocketAwait<Proto::FromServer::SessionInfo>{ ToString(FromClient::AddSession(domain, loginName, providerPK, userEndPoint, isSocket, requestId)), requestId, p, sl };
	}

	α Client::GraphQL( str query, SL sl )ε->ClientSocketAwait<string>{
		auto p = _pSession; THROW_IF( !p, "Not connected." );
		auto requestId = p->NextRequestId();
		Trace( sl, ELogTags::SocketClientWrite, "GraphQL: '{}'.", query.substr(0, Web::Client::MaxLogLength()) );
		return ClientSocketAwait<string>{ ToString(FromClient::GraphQL(query, requestId)), requestId, p, sl };
	}
namespace Client{
	StartSocketAwait::StartSocketAwait( SessionPK sessionId, SL sl )ι:base{sl}, _sessionId{ sessionId }{}

	α StartSocketAwait::Suspend()ι->void{
		_pSession = ms<Client::AppClientSocketSession>( Executor(), IsSsl() ? ssl::context(ssl::context::tlsv12_client) : optional<ssl::context>{} );
		[](StartSocketAwait& self, base::Handle h)->VoidTask {
			auto h2 = h;
			try{
				co_await _pSession->RunSession( Host(), Port() );//Web::Client
				[=]( base::Handle h )->ClientSocketAwait<Proto::FromServer::ConnectionInfo>::Task {
					try{
						co_await _pSession->Connect( self._sessionId );//handshake
					}
					catch( IException& e ){
						h.promise().SetError( move(e) );
					}
				}( h );
			}
			catch( IException& e ){
				h.promise().SetError( move(e) );
			}
			h.resume();
		}(*this, _h);
	}

	α AppClientSocketSession::Instance()ι->sp<AppClientSocketSession>{ return _pSession; }
	AppClientSocketSession::AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι:
		base( ioc, ctx )
	{}
	α AppClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::ConnectionInfo>{
		var requestId = NextRequestId();
		string instanceName = Settings::Get<string>("instanceName").value_or( "" );
		if( instanceName.empty() )
			instanceName = _debug ? "Debug" : "Release";
		return ClientSocketAwait<Proto::FromServer::ConnectionInfo>{ ToString(FromClient::Instance(Process::ApplicationName(), instanceName, sessionId, requestId)), requestId, shared_from_this(), sl };
	}

	α AppClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void {
			HandleException(move(h), CodeException{ec, _tags, ELogLevel::NoLog}, false );
		};
		CloseTasks( f );
		base::OnClose( ec );
		_pSession = nullptr;
		if( !Process::ShuttingDown() )
			App::Client::Connect();
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::SessionInfo>{
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::FromServer::SessionInfo>{ ToString(FromClient::Session(sessionId, requestId)), requestId, shared_from_this(), sl };
	}
	α AppClientSocketSession::GraphQL( string&& q, UserPK, SL sl )ι->ClientSocketAwait<string>{
		var requestId = NextRequestId();
		return ClientSocketAwait<string>{ ToString(FromClient::GraphQL(move(q), requestId)), requestId, shared_from_this(), sl };
	}

	template<class T,class... Args> α Resume( std::any&& hAny, T& v, fmt::format_string<Args const&...>&& m="", const Args&... args )ι->void{
		auto h = std::any_cast<typename ClientSocketAwait<T>::Handle>( &hAny );
		ASSERT( h );
		if( h ){
			h->promise().Log( FWD(m), FWD(args)... );
			h->promise().SetValue( move(v) );
			h->resume();
		}
	}
	ψ ResumeVoid( std::any&& h, const fmt::format_string<Args...> m="", Args&&... args )ι->void{
		auto p = std::any_cast<Web::Client::ClientSocketVoidAwait::Handle>( &h );
		p->promise().Log( m, std::forward<Args>(args)... );
		p->resume();
	}

	template<class T,class... Args> auto ResumeScaler( std::any&& h, T v, fmt::format_string<Args const&...>&& m="", const Args&... args )ι->void{
		Resume( move(h), v, FWD(m), FWD(args)... );
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

	α AppClientSocketSession::ProcessTransmission( Proto::FromServer::Transmission&& t, optional<UserPK> /*userPK*/, optional<RequestId> clientRequestId )ι->void{
		for( auto i=0; i<t.messages_size(); ++i ){
			auto m = t.mutable_messages( i );
			using enum Proto::FromServer::Message::ValueCase;
			var requestId = clientRequestId.value_or( m->request_id() );
			std::any hAny = requestId ? IClientSocketSession::PopTask( requestId ) : nullptr;
			switch( m->Value_case() ){
			[[unlikely]] case kAck:{
				var serverSocketId = m->ack();
				SetId( serverSocketId );
				Information( _tags, "[{:x}]AppClientSocketSession created: {}://{}.", Id(), IsSsl() ? "https" : "http", Host() );
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
				Resume( move(hAny), res, "SessionInfo: expiration: '{}', session_id: '{:x}', user_pk: '{}', user_endpoint: '{}'.", ToIsoString(IO::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				}break;
			case kGraphQl:
				Resume( move(hAny), *m->mutable_graph_ql(), "GraphQl: '{}'.", m->graph_ql().substr(0, Web::Client::MaxLogLength()) );
				break;
			case kException:
				HandleException( move(hAny), Jde::Proto::ToException(move(*m->mutable_exception())), requestId );
				break;
			case kExecute:
			case kExecuteAnonymous:{
				bool isAnonymous = m->Value_case()==kExecuteAnonymous;
				auto bytes = isAnonymous ? move( *m->mutable_execute_anonymous() ) : move( *m->mutable_execute()->mutable_transmission() );
				optional<UserPK> runAsPK = m->Value_case()==kExecuteAnonymous ? nullopt : optional<UserPK>(m->execute().user_pk() );
				LogRead( "Execute{} size: {:10L}", isAnonymous ? "Anonymous" : "", bytes.size()  );
				Execute( move(bytes), runAsPK, requestId );
				break;}
			case kExecuteResponse://wait for use case.
			case kStringPks://strings already saved in db, no need to send.  not being requested by client yet.
				Critical( _tags, "[{:x}]No use case has been implemented on client app '{}'.", Id(), underlying(m->Value_case()) );
			case kTraces:
			case kStatus:
				Critical( _tags, "[{:x}]Web only call not implemented on client app '{}'.", Id(), (uint)m->Value_case() );
			break;
			case VALUE_NOT_SET:
				break;
			}
		}
	}
	α AppClientSocketSession::HandleException( std::any&& h, IException&& e, RequestId requestId )ι->void{
		auto handle = [&]( sv /*msg*/, auto pAwait ){
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
			var severity{ requestId ? ELogLevel::Critical : ELogLevel::Debug };
			Log( severity, _tags, SRCE_CUR, "[0]Failed to process incoming exception '{}'.", requestId, e.what() );
		}
	}
	α AppClientSocketSession::WriteException( IException&& e, RequestId /*requestId*/ )->void{
		Write( FromClient::Exception(move(e)) );
	}
}}