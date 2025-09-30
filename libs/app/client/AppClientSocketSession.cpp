#include <jde/app/client/AppClientSocketSession.h>
#include <jde/fwk/process/execution.h>
#include <jde/web/client/socket/ClientQL.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/clientSubscriptions.h>
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/app/shared/proto/Common.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>

#define let const auto

namespace Jde::App{
	using Web::Client::ClientSocketAwait; using Jde::Proto::ToString;
	constexpr ELogTags _tags{ ELogTags::SocketClientRead };

namespace Client{
	StartSocketAwait::StartSocketAwait( SessionPK sessionId, sp<Access::Authorize> authorize, sp<IAppClient> appClient, SL sl )ι:
		base{sl},
		_appClient{ appClient },
		_authorize{ authorize },
		_sessionId{ sessionId },
		_session{ ms<Client::AppClientSocketSession>(Executor(), IsSsl() ? ssl::context(ssl::context::tlsv12_client) : optional<ssl::context>{}, move(_authorize), move(appClient)) }
	{}

	α StartSocketAwait::Suspend()ι->void{
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
			auto info = co_await _session->Connect( _sessionId );//handshake
			_appClient->SetInstancePK( info.instance_pk() );
			_appClient->SetSession( move(_session) );
			_appClient->ServerPublicKey = {
				{ info.certificate_modulus().begin(), info.certificate_modulus().end() },
				{ info.certificate_exponent().begin(), info.certificate_exponent().end() }
			};
			Resume( move(info) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	//α AppClientSocketSession::Instance()ι->sp<AppClientSocketSession>{ return _session; }
	AppClientSocketSession::AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx, sp<Access::Authorize> authorize, sp<IAppClient> appClient )ι:
		base( ioc, ctx ),
		_appClient{appClient},
		_authorize{authorize}
	{}

	α AppClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::ConnectionInfo>{
		let requestId = NextRequestId();
		auto instanceName = Settings::FindString("instanceName").value_or( "" );
		if( instanceName.empty() )
			instanceName = _debug ? "Debug" : "Release";
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]Connect: '{}'.", requestId, instanceName );
		return ClientSocketAwait<Proto::FromServer::ConnectionInfo>{ ToString(FromClient::Instance(Process::ApplicationName(), instanceName, sessionId, requestId)), requestId, shared_from_this(), sl };
	}

	α AppClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void {
			HandleException(move(h), CodeException{ec, _tags, ELogLevel::NoLog}, false );
		};
		CloseTasks( f );
		base::OnClose( ec );
		_appClient->SetSession( nullptr );
		if( !Process::ShuttingDown() )
			App::Client::Connect( move(_appClient) );
	}
	α AppClientSocketSession::SessionInfo( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Web::FromServer::SessionInfo>{
		let requestId = NextRequestId();
		return ClientSocketAwait<Web::FromServer::SessionInfo>{ FromClient::Session(sessionId, requestId), requestId, shared_from_this(), sl };
	}
	// α AppClientSocketSession::SessionInfo( Web::Jwt&& jwt, SL sl )ι->ClientSocketAwait<Web::FromServer::SessionInfo>{
	// 	let requestId = NextRequestId();
	// 	return ClientSocketAwait<Web::FromServer::SessionInfo>{ ToString(FromClient::Session(move(jwt), requestId)), requestId, shared_from_this(), sl };
	// }
	α AppClientSocketSession::Query( string&& q, bool returnRaw, SL sl )ι->ClientSocketAwait<jvalue>{
		let requestId = NextRequestId();
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]GraphQL: '{}'.", requestId, q.substr(0, Web::Client::MaxLogLength()) );

		return ClientSocketAwait<jvalue>{ ToString(FromClient::Query(move(q), requestId, returnRaw)), requestId, shared_from_this(), sl };
	}
	concurrent_flat_map<RequestId, std::pair<sp<QL::IListener>,vector<QL::Subscription>>> _subscriptionRequests;
	α AppClientSocketSession::Subscribe( string&& q, sp<QL::IListener> listener, SL sl )ε->await<jarray>{
		let requestId = NextRequestId();
		LOGSL( ELogLevel::Trace, sl, ELogTags::SocketClientWrite, "[{:x}]Subscribe: '{}'.", requestId, q.substr(0, Web::Client::MaxLogLength()) );
		auto subscriptions = QL::ParseSubscriptions( q, _appClient->SubscriptionSchemas, sl );
		_subscriptionRequests.emplace( requestId, make_pair(listener, move(subscriptions)) );
		return ClientSocketAwait<jarray>{ ToString(FromClient::Subscription(move(q), requestId)), requestId, shared_from_this(), sl };
	}

	template<class T,class... Args> Ω resume( std::any&& hAny, T&& v/*, fmt::format_string<Args const&...>&& m="", const Args&... args*/ )ι->void{
		auto h = std::any_cast<typename ClientSocketAwait<T>::Handle>( &hAny );
		ASSERT_DESC( h, Ƒ("typeT={}, typeV={}", typeid(typename ClientSocketAwait<T>::Handle).name(), hAny.type().name()) );
		if( h ){
			h->promise().SetValue( move(v) );
			h->resume();
		}
	}

	template<class... Args> Ω resumeJValue( std::any&& hAny, string&& v )ι->void{
		try{
			resume<jvalue>( move(hAny), Json::ParseValue(move(v)) );
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
		//h->promise().Log( FWD(m), FWD(args)... );
		h->resume();
	}

	template<class T,class... Args>
	α resumeScaler( std::any&& h, T v )ι->void{
		resume( move(h), move(v) );
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
					_qlServer = ms<Web::Client::ClientQL>( shared_from_this(), move(_authorize) );
				INFO( "[{:x}]AppClientSocketSession created: {}://{}.", Id(), IsSsl() ? "https" : "http", Host() );
//				resumeVoid( move(hAny), "Ack: '{}'.", serverSocketId );
				}break;
			case kConnectionInfo:
				TRACE( "[{:x}]ConnectionInfo: applicationInstance: '{}'.", Id(), m->connection_info().instance_pk() );
				resume( move(hAny), move(*m->mutable_connection_info()) );
				break;
			case kGeneric:
				TRACE( "[{:x}]Generic: '{}'.", Id(), m->generic() );
				resume( move(hAny), move(*m->mutable_generic()) );
				break;
			[[likely]] case kStrings:{
				auto& res = *m->mutable_strings();
				TRACE( "[{:x}]Strings: count='{}'.", Id(), res.messages().size()+res.files().size()+res.functions().size()+res.threads().size() );
				resume( move(hAny), move(*m->mutable_strings()) );
				}break;
			case kJwt:
				TRACE( "[{:x}]Jwt: size='{}'.", Id(), m->jwt().size() );
				resume( move(hAny), Web::Jwt{move(*m->mutable_jwt())} );
				break;
/*			case kLogLevels:{//TODO implement when have tags.
				auto& res = *m->mutable_log_levels();
				TRACE( "[{:x}]LogLevels: server='{}', client='{}'.", Id(), ToString((ELogLevel)res.server()), ToString((ELogLevel)res.client()) );
				resume( move(hAny), move(res) );
				}break;*/
			case kProgress://TODO not awaitable
				TRACE( "[{:x}]Progress: '{}'.", Id(), m->progress() );
				resumeScaler( move(hAny), m->progress() );
				break;
			case kSessionInfo:{
				auto& res = *m->mutable_session_info();
				TRACE( "[{:x}]SessionInfo: expiration: '{}', session_id: '{:x}', user_pk: '{}', user_endpoint: '{}'.", Id(), ToIsoString(Jde::Proto::ToTimePoint(res.expiration())), res.session_id(), res.user_pk(), res.user_endpoint() );
				resume( move(hAny), move(res) );
				}break;
			case kGraphQl:
				TRACE( "[{:x}]GraphQl: '{}'.", Id(), m->graph_ql().substr(0, Web::Client::MaxLogLength()) );
				resumeJValue( move(hAny), move(*m->mutable_graph_ql()) );
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
					TRACE( "[{:x}]SubscriptionAck: '{}'.", Id(), serialize(y) );
					resume( move(hAny), move(y) );
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
				CRITICAL( "[{:x}]No use case has been implemented on client app '{}'.", Id(), underlying(m->Value_case()) );
				break;
			case kTraces:
			case kStatus:
				CRITICAL( "[{:x}]Web only call not implemented on client app '{}'.", Id(), (uint)m->Value_case() );
			break;
			case VALUE_NOT_SET:
				break;
			}
		}
	}
	α AppClientSocketSession::HandleException( std::any&& h, IException&& e, RequestId requestId )ι->void{
		auto handle = [&]( sv /*msg*/, auto await ){
			await->promise().ResponseMessage = "Error: {}";
			await->promise().MessageArgs.emplace_back( e.what() );
			await->promise().SetExp( move(e) );
			await->resume();
		};
		if( auto await = std::any_cast<ClientSocketAwait<uint32>::Handle>(&h) )
			handle( "Exception<uint32>: '{}'.", await );
		else if( auto await = std::any_cast<ClientSocketAwait<string>::Handle>(&h) )
			handle( "Exception<string>: '{}'.", await );
		else if( auto await = std::any_cast<ClientSocketAwait<Proto::FromServer::Strings>::Handle>(&h) )
			handle( "Exception<Strings>: '{}'.", await );
		else if( auto await = std::any_cast<ClientSocketAwait<Web::FromServer::SessionInfo>::Handle>(&h) )
			handle( "Exception<SessionInfo>: '{}'.", await );
		else if( auto await = std::any_cast<ClientSocketAwait<jvalue>::Handle>(&h) )
			handle( "Exception<jvalue>: '{}'.", await );
		else if( auto await = std::any_cast<ClientSocketAwait<Web::Jwt>::Handle>(&h) )
			handle( "Exception<Jwt>: '{}'.", await );
		else{
			let severity{ requestId ? ELogLevel::Critical : ELogLevel::Debug };
			ASSERT_DESC( !requestId, Ƒ("Type Not Expected={}", h.type().name()) );
			LOG( severity, _tags, "[{:x}]Failed to process incoming exception '{}'.", requestId, e.what() );
		}
	}
	α AppClientSocketSession::WriteException( exception&& e, RequestId requestId )ι->void{
		Write( FromClient::Exception(move(e), requestId) );
	}
	α AppClientSocketSession::WriteException( string&& e, RequestId requestId )ι->void{
		Write( FromClient::Exception(move(e), requestId) );
	}
}}