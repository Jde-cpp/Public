#include "WebServer.h"
#include <jde/web/server/Server.h>
#include <jde/app/client/IAppClient.h> //!important
#include "GatewaySocketSession.h"
#include "HttpRequestAwait.h"
#include "StartupAwait.h"
#include "ql/GatewayQL.h"

namespace Jde::Opc{
	optional<std::jthread> _serverThread;
	concurrent_flat_map<uint,sp<Opc::Gateway::GatewaySocketSession>> _sessions; // Consider using server
	static sp<Gateway::RequestHandler> _requestHandler;
	α Gateway::StartWebServer( jobject&& settings )ε->void{
		_requestHandler = ms<RequestHandler>( move(settings), AppClient() );
		Web::Server::Start( _requestHandler );
		Process::AddShutdownFunction( [](bool terminate ){StopWebServer(terminate); } );//TODO move to Web::Server
	}

	α Gateway::StopWebServer( bool terminate )ι->void{
		Web::Server::Stop( move(_requestHandler), terminate );
	}
namespace Gateway{
	α Server::RemoveSession( uint socketSessionId )ι->void{
		_sessions.erase( socketSessionId );
	}

	using namespace Jde::Web::Server;
	α RequestHandler::HandleRequest( HttpRequest&& req, SL sl )ι->up<IHttpRequestAwait>{
		return mu<HttpRequestAwait>( move(req), sl );
	}

	α RequestHandler::WebsocketSession( sp<Web::Server::RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<Web::Server::IWebsocketSession>{
		auto session = ms<Gateway::GatewaySocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}
	α RequestHandler::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return Gateway::Schemas(); }
}}