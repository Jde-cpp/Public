#include "WebServer.h"
#include "../AppServer.h"
//#include "ServerSocketSession.h"
#include <jde/web/server/Server.h>
#include "../StartupAwait.h"

namespace Jde::Opc{
	//concurrent_flat_map<uint,sp<Opc::ServerSocketSession>> _sessions; // Consider using server

	α Server::StartWebServer( jobject&& settings )ε->void{
		Web::Server::Start( mu<RequestHandler>(), mu<AppServer>(), move(settings) );
		Process::AddShutdownFunction( [](bool /*terminate*/ ){StopWebServer();} );//TODO move to Web::Server
	}
	α Server::StopWebServer()ι->void{
			Web::Server::Stop();
	}
	namespace Server{
		α RequestHandler::GetWebsocketSession( sp<RestStream>&& /*stream*/, beast::flat_buffer&& /*buffer*/, TRequestType /*req*/, tcp::endpoint /*userEndpoint*/, uint32 /*connectionIndex*/ )ι->sp<IWebsocketSession>{
			ASSERT_DESC( false, "Websocket sessions not implemented in OpcServer." );
			// auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
			// _sessions.emplace( session->Id(), session );
			return nullptr;
		}
		α RequestHandler::Schemas()ι->const vector<sp<DB::AppSchema>>&{
			return Opc::Server::Schemas();
		}
	}
}