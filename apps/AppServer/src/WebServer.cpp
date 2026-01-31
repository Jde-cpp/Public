#include "WebServer.h"
#include <jde/ql/types/FilterQL.h>
#include <jde/app/IApp.h>
#include "LocalClient.h"
#include "ServerSocketSession.h"
#include "ql/AppQL.h"
#include "ql/AppQLAwait.h"

#define let const auto
namespace Jde::App::Server{
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
}
namespace Jde::App{
	using QL::Filter;
	concurrent_flat_map<uint32,sp<Server::ServerSocketSession>> _sessions; //Consider using main class+ql subscriptions
	concurrent_flat_map<ProgInstPK,Filter> _logSubscriptions;
	//concurrent_flat_map<ProgInstPK,Proto::FromServer::Status> _statuses;
	concurrent_flat_set<ProgInstPK> _statusSubscriptions;

	ProgramPK _appId;
	ProgInstPK _instancePK;
	atomic<RequestId> _requestId{ 0 };
	sp<Server::RequestHandler> _requestHandler;
	α Server::GetRequestHandler()ι->sp<RequestHandler>{ return _requestHandler; }
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return Server::QL().Schemas(); }

	α Server::GetAppPK()ι->ProgramPK{ return _appId; }
	α Server::SetAppPKs( std::tuple<ProgramPK, ProgInstPK, ConnectionPK> x )ι->void{
		_appId=get<0>( x );
		AppClient()->SetAppPKs( get<1>(x), get<2>(x) );
	}
	α Server::StartWebServer( jobject&& settings )ε->void{
		_requestHandler = ms<RequestHandler>( move(settings) );
		Web::Server::Start( _requestHandler );
		Process::AddShutdownFunction( [](bool terminate){Server::StopWebServer(terminate);} );//TODO move to Web::Server
	}
	α Server::RemoveExisting( str host, PortType port )ι->void{
		_sessions.erase_if( [&host=host,port=port](auto&& kv){
			auto& existing = kv.second->Instance();
			return existing.host()==host && existing.web_port()==port;
		});
	}

	α Server::GetJwt( UserPK userPK, string name, string target, string endpoint, SessionPK sessionId, TimePoint expires, string description )ι->Web::Jwt{
		auto requestHandler = _requestHandler;
		THROW_IF( !requestHandler, "No request Handler." );
		return requestHandler->Jwt( userPK, move(name), move(target), move(endpoint), sessionId, expires, move(description) );
	}

	α Server::StopWebServer( bool terminate )ι->void{
		Web::Server::Stop( move(_requestHandler), terminate );
	}

	α Server::FindApplications( str name )ι->vector<Proto::FromClient::Instance>{
		vector<Proto::FromClient::Instance> y;
		_sessions.visit_all( [&](auto&& kv){
			auto& session = kv.second;
			if( let instance = session->Instance(); instance.application()==name )
				y.push_back( instance );
		} );
		return y;
	}

	α Server::FindConnection( ConnectionPK connectionPK )ι->sp<ServerSocketSession>{
		sp<ServerSocketSession> y;
		_sessions.visit( connectionPK, [&](auto&& kv){y=kv.second;} );
		return y;
	}

	α Server::NextRequestId()->RequestId{ return ++_requestId; }
	α Server::QuerySessions( QL::TableQL ql, UserPK executer, SL sl )ι->QuerySessionsAwait{
		vector<sp<ServerSocketSession>> sessions;
		_sessions.visit_all( [&](auto&& kv){
			sessions.push_back( kv.second );
		});
		return QuerySessionsAwait{ move(ql), executer, move(sessions), sl };
	}
	α Server::Write( ProgramPK appPK, optional<ConnectionPK> connectionPK, Proto::FromServer::Transmission&& msg )ε->void{
		if( !_sessions.visit_while([&](auto&& kv){
			auto& session = kv.second;
			auto appInstPK = session->ProgramPK()==appPK ? session->ConnectionPK() : 0;
			let found = appInstPK && appInstPK==connectionPK.value_or( appInstPK );
			if( found )
				session->Write( move(msg) );
			return !found;
		}) ){
			THROW( "No session found for appPK:{}, connectionPK:{}", appPK, connectionPK.value_or(0) );
		}
	}
	α Server::RemoveSession( ConnectionPK connectionPK )ι->void{
		_logSubscriptions.erase( connectionPK );
		ForwardExecutionAwait::OnCloseConnection( connectionPK );
		TRACET( ELogTags::App, "[{:x}]RemoveSession", connectionPK );
	}

}
namespace Jde::App::Server{
	RequestHandler::RequestHandler( jobject&& settings )ι:
		IRequestHandler{ move(settings), Server::AppClient() }
	{}

	α RequestHandler::Jwt( UserPK userPK, string&& name, string&& target, string&& endpoint, SessionPK sessionId, TimePoint expires, string&& description )ι->Web::Jwt{
		auto publicKey = Crypto::ReadPublicKey( Settings().Crypto().PublicKeyPath );
		return Web::Jwt{ move(publicKey), userPK, move(name), move(target), sessionId, move(endpoint), expires, move(description), Settings().Crypto().PrivateKeyPath };
	}

	α RequestHandler::Query( QL::RequestQL&& ql, UserPK executer, bool raw, SL sl )ι->up<TAwait<jvalue>>{
		return mu<AppQLAwait>( move(ql), executer, raw, sl );
	}

	α RequestHandler::WebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession>{
		auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}
}