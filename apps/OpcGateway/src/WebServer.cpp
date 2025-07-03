#include "WebServer.h"
#include <jde/web/server/Server.h>
#include "ServerSocketSession.h"
#include "ApplicationServer.h"

namespace Jde{
	optional<std::jthread> _serverThread;
	concurrent_flat_map<uint,sp<Opc::ServerSocketSession>> _sessions; // Consider using server

	α Opc::StartWebServer()ε->void{
		Web::Server::Start( mu<RequestHandler>(), mu<ApplicationServer>() );
		Process::AddShutdownFunction( [](bool /*terminate*/ ){Opc::StopWebServer();} );//TODO move to Web::Server
	}

	α Opc::StopWebServer()ι->void{
		Web::Server::Stop();
	}
namespace Opc{
	α Server::RemoveSession( uint socketSessionId )ι->void{
		_sessions.erase( socketSessionId );
	}

	α RequestHandler::GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession>{
		auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}
}}