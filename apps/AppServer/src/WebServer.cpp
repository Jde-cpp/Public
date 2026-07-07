#include "WebServer.h"
#include <jde/ql/types/FilterQL.h>
#include <jde/app/IApp.h>
#include "LocalClient.h"
#include "ServerSocketSession.h"
#include "jde/fwk/usings.h"
#include "ql/AppServerQL.h"
#include "LogData.h"

#define let const auto
namespace Jde::App::Server{
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
}
namespace Jde::App{
	using QL::Filter;
	concurrent_flat_map<uint32,sp<Server::ServerSocketSession>> _sessions; //Consider using main class+ql subscriptions

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
		Process::AddShutdownFunction( [](bool terminate, SL sl){Server::StopWebServer(terminate, sl);} );//TODO move to Web::Server
	}
	α Server::RemoveExisting( str host, PortType port )ι->void{
		_sessions.erase_if( [&host=host,port=port](auto&& kv){
			auto& existing = kv.second->Instance();
			return existing.host()==host && existing.web_port()==port;
		});
	}
	α Server::OnSessionDisconnect( sp<ServerSocketSession> session )ι->void{
		_sessions.erase( session->Id() );
		ForwardExecutionAwait::OnCloseConnection( session->ConnectionPK() );
		EndInstance( session->ConnectionPK() );
	}

	α Server::GetJwt( UserPK userPK, string name, string target, string endpoint, SessionPK sessionId, TimePoint expires, string description )ε->Web::Jwt{
		auto requestHandler = _requestHandler;
		THROW_IF( !requestHandler, "No request Handler." );
		return requestHandler->Jwt( userPK, move(name), move(target), move(endpoint), sessionId, expires, move(description) );
	}

	α Server::StopWebServer( bool terminate, SL sl )ι->void{
		Web::Server::Stop( move(_requestHandler), terminate, sl );
	}

	α Server::FindApplications( str name )ι->vector<Proto::FromClient::Instance>{
		vector<Proto::FromClient::Instance> y;
		_sessions.cvisit_all( [&](let& kv){
			let& session = kv.second;
			if( let instance = session->Instance(); instance.application()==name )
				y.push_back( instance );
		} );
		return y;
	}

	α Server::FindConnection( ConnectionPK connectionPK )ι->sp<ServerSocketSession>{
		sp<ServerSocketSession> y;
		if( connectionPK ){ //_sessions is keyed by socket Id, not ConnectionPK; 0=session not yet registered.
			_sessions.cvisit_while( [&](let& kv){
				if( kv.second->ConnectionPK()==connectionPK )
					y = kv.second;
				return !y;
			});
		}
		return y;
	}

	α Server::NextRequestId()->RequestId{ return ++_requestId; }
	α Server::QuerySessions( QL::TableQL ql, UserPK executer, SL sl )ι->QuerySessionsAwait{
		vector<sp<ServerSocketSession>> sessions;
		_sessions.visit_all( [&](auto&& kv){
			if( kv.second->ConnectionPK() )//skip not-yet-registered sessions: no status to report and they share ConnectionPK 0.
				sessions.push_back( kv.second );
		});
		return QuerySessionsAwait{ move(ql), executer, move(sessions), sl };
	}
	α Server::Write( ProgramPK appPK, optional<ConnectionPK> connectionPK, Proto::FromServer::Transmission&& msg )ε->void{
		if( _sessions.visit_while([&](auto&& kv){ //visit_while returns true if no lambda returned false, i.e. no session matched.
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
}
namespace Jde::App::Server{
	RequestHandler::RequestHandler( jobject&& settings )ι:
		IRequestHandler{ move(settings), Server::AppClient() }
	{}

	α RequestHandler::Jwt( UserPK userPK, string&& name, string&& target, string&& endpoint, SessionPK sessionId, TimePoint expires, string&& description )ι->Web::Jwt{
		auto publicKey = Crypto::ReadPublicKey( Settings().Crypto().PublicKeyPath );
		return Web::Jwt{ move(publicKey), userPK, move(name), move(target), sessionId, move(endpoint), expires, move(description), Settings().Crypto().PrivateKeyPath };
	}

	α RequestHandler::WebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession>{
		auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}
	α RequestHandler::QLServer()ι->sp<QL::IQL>{
		return QLPtr();
	}
}