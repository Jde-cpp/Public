#include <jde/app/client/AppClientSocketSession.h>
#include <jde/framework/thread/execution.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/app/shared/proto/Common.h>
#include <jde/web/client/socket/ClientQL.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/clientSubscriptions.h>
#include <jde/app/client/appClient.h>
#include <jde/app/shared/StringCache.h>

#define let const auto

namespace Jde::App{
	using Web::Client::ClientSocketAwait; using Jde::Proto::ToString;
	constexpr ELogTags _tags{ ELogTags::SocketClientRead };

	sp<Client::AppClientSocketSession> _session;
	α Client::CloseSocketSession( SL sl )ι->VoidTask{
		if( _session ){
			let tags = ELogTags::Client | ELogTags::Socket;
			Trace( sl, tags, "ClosingSocketSession" );
			co_await _session->Close();
			_session = nullptr;
			Information( sl, tags, "ClosedSocketSession" );
		}
	}
	α Client::AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, SL sl )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>{
		auto p = _session; THROW_IF( !p, "Not connected." );
		auto requestId = p->NextRequestId();
		Trace( sl, ELogTags::SocketClientWrite, "AddSession domain: '{}', loginName: '{}', providerPK: {}, userEndPoint: '{}', isSocket: {}.", domain, loginName, providerPK, userEndPoint, isSocket );
		return ClientSocketAwait<Web::FromServer::SessionInfo>{ ToString(FromClient::AddSession(domain, loginName, providerPK, userEndPoint, isSocket, requestId)), requestId, p, sl };
	}

namespace Client{
	StartSocketAwait::StartSocketAwait( SessionPK sessionId, SL sl )ι:base{sl}, _sessionId{ sessionId }{}

	α StartSocketAwait::Suspend()ι->void{
		_session = ms<Client::AppClientSocketSession>( Executor(), IsSsl() ? ssl::context(ssl::context::tlsv12_client) : optional<ssl::context>{} );
		RunSession();
	}
	α StartSocketAwait::RunSession()ι->VoidTask{
		try{
			co_await _session->RunSession( Host(), Port() );//Web::Client
			SendSessionId();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α StartSocketAwait::SendSessionId()ι->ClientSocketAwait<Proto::FromServer::ConnectionInfo>::Task{
		try{
			auto connectionInfo =  co_await _session->Connect( _sessionId );//handshake
			Resume( move(connectionInfo) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α AppClientSocketSession::Instance()ι->sp<AppClientSocketSession>{ return _session; }
	AppClientSocketSession::AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι:
		base( ioc, ctx )
	{}
	α AppClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::ConnectionInfo>{
		let requestId = NextRequestId();
		auto instanceName = Settings::FindString("instanceName").value_or( "" );
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
		_session = nullptr;
		if( !Process::ShuttingDown() )
			App::Client::Connect();
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Web::FromServer::SessionInfo>{
		let requestId = NextRequestId();
		return ClientSocketAwait<Web::FromServer::SessionInfo>{ ToString(FromClient::Session(sessionId, requestId)), requestId, shared_from_this(), sl };
	}
	α AppClientSocketSession::Query( string&& q, SL sl )ι->ClientSocketAwait<jvalue>{
		let requestId = NextRequestId();
		Trace( sl, ELogTags::SocketClientWrite, "[{:x}]GraphQL: '{}'.", requestId, q.substr(0, Web::Client::MaxLogLength()) );

		return ClientSocketAwait<jvalue>{ ToString(FromClient::Query(move(q), requestId)), requestId, shared_from_this(), sl };
	}
	concurrent_flat_map<RequestId, std::pair<sp<QL::IListener>,vector<QL::Subscription>>> _subscriptionRequests;
	α AppClientSocketSession::Subscribe( string&& q, sp<QL::IListener> listener, SL sl )ε->await<jarray>{
		let requestId = NextRequestId();
		Trace( sl, ELogTags::SocketClientWrite, "[{:x}]Subscribe: '{}'.", requestId, q.substr(0, Web::Client::MaxLogLength()) );
		auto subscriptions = QL::ParseSubscriptions(q);
		_subscriptionRequests.emplace( requestId, make_pair(listener, move(subscriptions)) );
		return ClientSocketAwait<jarray>{ ToString(FromClient::Subscription(move(q), requestId)), requestId, shared_from_this(), sl };
	}

	template<class T,class... Args> Ω resume( std::any&& hAny, T&& v, fmt::format_string<Args const&...>&& m="", const Args&... args )ι->void{
		auto h = std::any_cast<typename ClientSocketAwait<T>::Handle>( &hAny );
		ASSERT_DESC( h, Ƒ("typeT={}, typeV={}", typeid(typename ClientSocketAwait<T>::Handle).name(), hAny.type().name()) );
		if( h ){
			h->promise().Log( FWD(m), FWD(args)... );
			h->promise().SetValue( move(v) );
			h->resume();
		}
	}

	template<class... Args> Ω resumeJValue( std::any&& hAny, string&& v, fmt::format_string<Args const&...>&& m="", const Args&... args )ι->void{
		try{
			resume<jvalue>( move(hAny), Json::ParseValue(move(v)), FWD(m), FWD(args)... );
		}
		catch( IException& e ){
			if( auto h = std::any_cast<typename ClientSocketAwait<jvalue>::Handle>(&hAny); h ){
				h->promise().SetExp( move(e) );
				h->resume();
			}
			else
				ASSERT_DESC( false, "hAny is null for jvalue." );
		}
	}

	ψ resumeVoid( std::any&& hAny, const fmt::format_string<Args...> m="", Args&&... args )ι->void{
		auto h = std::any_cast<Web::Client::ClientSocketVoidAwait::Handle>( &hAny );
		ASSERT_DESC( h, Ƒ("typeT={}, typeV={}", typeid(Web::Client::ClientSocketVoidAwait::Handle).name(), hAny.type().name()) );
		h->promise().Log( FWD(m), FWD(args)... );
		h->resume();
	}

	template<class T,class... Args>
	α ResumeScaler( std::any&& h, T v, fmt::format_string<Args const&...>&& m="", const Args&... args )ι->void{
		resume( move(h), move(v), FWD(m), FWD(args)... );
	}

	α AppClientSocketSession::Execute( string&& bytes, optional<Jde::UserPK> userPK, RequestId clientRequestId )ι->void{
		try{
			auto t = Jde::Proto::Deserialize<Proto::FromServer::Transmission>( move(bytes) );
			ProcessTransmission( move(t), userPK, clientRequestId );
		}
		catch( IException& e ){
			WriteException( move(e), clientRequestId );
		}
	}

	α AppClientSocketSession::OnRead( Proto::FromServer::Transmission&& t )ι->void{
		ProcessTransmission( move(t), _userPK, nullopt );
	}

	α AppClientSocketSession::ProcessTransmission( Proto::FromServer::Transmission&& t, optional<Jde::UserPK> /*userPK*/, optional<RequestId> clientRequestId )ι->void{
		for( auto i=0; i<t.messages_size(); ++i ){
			auto m = t.mutable_messages( i );
			using enum Proto::FromServer::Message::ValueCase;
			let requestId = clientRequestId.value_or( m->request_id() );
			std::any hAny = requestId ? IClientSocketSession::PopTask( requestId ) : nullptr;
			switch( m->Value_case() ){
			[[unlikely]] case kAck:{
				let serverSocketId = m->ack();
				SetId( serverSocketId );
				if( !_qlServer )
					_qlServer = ms<Web::Client::ClientQL>( shared_from_this() );
				Information( _tags, "[{:x}]AppClientSocketSession created: {}://{}.", Id(), IsSsl() ? "https" : "http", Host() );
//				resumeVoid( move(hAny), "Ack: '{}'.", serverSocketId );
				}break;
			case kGeneric:
				resume( move(hAny), move(*m->mutable_generic()), "Generic - '{}'.", m->generic() );
				break;
			[[likely]] case kStrings:{
				auto& res = *m->mutable_strings();
				resume( move(hAny), move(*m->mutable_strings()), "Strings count='{}'.", res.messages().size()+res.files().size()+res.functions().size()+res.threads().size() );
				}break;
			case kLogLevels:{//TODO implement when have tags.
				auto& res = *m->mutable_log_levels();
				resume( move(hAny), move(res), "LogLevel server='{}', client='{}'.", ToString((ELogLevel)res.server()), ToString((ELogLevel)res.client()) );
				}break;
			case kProgress://TODO not awaitable
				ResumeScaler( move(hAny), m->progress(), "Progress: '{}'.", m->progress() );
				break;
			case kSessionInfo:{
				auto& res = *m->mutable_session_info();
				resume( move(hAny), move(res), "SessionInfo: expiration: '{}', session_id: '{:x}', user_pk: '{}', user_endpoint: '{}'.", ToIsoString(Jde::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				}break;
			case kGraphQl:
				resumeJValue( move(hAny), move(*m->mutable_graph_ql()), "GraphQl: '{}'.", m->graph_ql().substr(0, Web::Client::MaxLogLength()) );
				break;
			case kSubscriptionAck:
				if( !_subscriptionRequests.erase_if( requestId, [&](auto&& kv){
					auto& listenerSubs = kv.second;
					Web::Client::Subscriptions::ListenRemote( listenerSubs.first, move(listenerSubs.second) );
					return true;
				}) ){
					HandleException( move(hAny), Exception{ "SubscriptionAck: '{}' not found.", requestId }, requestId );
				}
				else{
					jarray y;
					for_each( m->subscription_ack().server_ids(), [&]( auto id ){ y.emplace_back(id); } );
					resume( move(hAny), move(y), "SubscriptionAck: '{}'.", serialize(y) );
				}
				break;
			[[likely]]case kSubscription:
				OnMessage( move(*m->mutable_subscription()), requestId );
			break;
			case kException:
				HandleException( move(hAny), Jde::Proto::ToException(move(*m->mutable_exception())), requestId );
				break;
			case kExecute:
			case kExecuteAnonymous:{
				bool isAnonymous = m->Value_case()==kExecuteAnonymous;
				auto bytes = isAnonymous ? move( *m->mutable_execute_anonymous() ) : move( *m->mutable_execute()->mutable_transmission() );
				optional<Jde::UserPK> runAsPK = m->Value_case()==kExecuteAnonymous ? nullopt : optional<Jde::UserPK>( {m->execute().user_pk()} );
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
			pAwait->promise().SetExp( move(e) );
			pAwait->resume();
		};
		if( auto pAwait = std::any_cast<ClientSocketAwait<uint32>::Handle>(&h) )
			handle( "Exception<uint32>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<string>::Handle>(&h) )
			handle( "Exception<string>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Proto::FromServer::Strings>::Handle>(&h) )
			handle( "Exception<Strings>: '{}'.", pAwait );
		else if( auto pAwait = std::any_cast<ClientSocketAwait<Web::FromServer::SessionInfo>::Handle>(&h) )
			handle( "Exception<SessionInfo>: '{}'.", pAwait );
		else{
			let severity{ requestId ? ELogLevel::Critical : ELogLevel::Debug };
			Log( severity, _tags, SRCE_CUR, "[0]Failed to process incoming exception '{}'.", requestId, e.what() );
		}
	}
	α AppClientSocketSession::WriteException( exception&& e, RequestId /*requestId*/ )->void{
		Write( FromClient::Exception(move(e)) );
	}
}}