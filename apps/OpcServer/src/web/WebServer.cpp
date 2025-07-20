#include "WebServer.h"
#include "../OpcServerAppClient.h"
#include <jde/web/server/Server.h>
#include "../StartupAwait.h"

namespace Jde::Opc{
	sp<Server::RequestHandler> _requestHandler;

	α Server::StartWebServer( jobject&& settings )ε->void{
		_requestHandler = ms<RequestHandler>( move(settings), AppClient() );
		Web::Server::Start( _requestHandler );
		Process::AddShutdownFunction( [](bool terminate ){StopWebServer(terminate);} );//TODO move to Web::Server
	}
	α Server::StopWebServer( bool terminate )ι->void{
		Web::Server::Stop( move(_requestHandler), terminate );
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