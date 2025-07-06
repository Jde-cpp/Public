#include "WebServer.h"
#include <jde/web/server/Server.h>
#include "GatewaySocketSession.h"
#include "AppServer.h"
#include "HttpRequestAwait.h"
#include "StartupAwait.h"

namespace Jde{
	optional<std::jthread> _serverThread;
	concurrent_flat_map<uint,sp<Opc::Gateway::GatewaySocketSession>> _sessions; // Consider using server

	α Opc::StartWebServer( jobject&& settings )ε->void{
		Web::Server::Start( mu<RequestHandler>(), mu<ApplicationServer>(), move(settings) );
		Process::AddShutdownFunction( [](bool /*terminate*/ ){Opc::StopWebServer();} );//TODO move to Web::Server
	}

	α Opc::StopWebServer()ι->void{
		Web::Server::Stop();
	}
namespace Opc{
	α Server::RemoveSession( uint socketSessionId )ι->void{
		_sessions.erase( socketSessionId );
	}

	using namespace Jde::Web::Server;
	α RequestHandler::HandleRequest( HttpRequest&& req, SL sl )ι->up<IHttpRequestAwait>{
		return mu<HttpRequestAwait>( move(req), sl );
	}

	α RequestHandler::GetWebsocketSession( sp<Web::Server::RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession>{
		auto session = ms<Gateway::GatewaySocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}
	α RequestHandler::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return Gateway::Schemas(); }
}}