#pragma once
#include <jde/web/server/IRequestHandler.h>
#include <jde/app/client/IAppClient.h>
#include "HttpRequestAwait.h"


namespace Jde::Opc::Server{
	α StartWebServer( jobject&& settings )ε->void;
	α StopWebServer( bool terminate )ι->void;
	namespace Server{
		α RemoveSession( uint socketSessionId )ι->void;
	}
	struct RequestHandler final : IRequestHandler{
		RequestHandler( jobject settings, sp<App::Client::IAppClient> appServer )ι: IRequestHandler{ move(settings), move(appServer) }{}
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override;
	};
}