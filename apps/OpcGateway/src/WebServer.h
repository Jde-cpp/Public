#pragma once
#include <jde/web/server/IRequestHandler.h>
#include "HttpRequestAwait.h"
#include <jde/web/server/IApplicationServer.h>

namespace Jde::Opc{
	α StartWebServer()ε->void;
	α StopWebServer()ι->void;
namespace Server{
	α RemoveSession( uint socketSessionId )ι->void;
}
	struct RequestHandler final : IRequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
	};
}