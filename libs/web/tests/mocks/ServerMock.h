#pragma once
#include "HttpRequestAwait.h"
#include <jde/web/server/IRequestHandler.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Server;
	const string Host{ "localhost" };
	constexpr PortType Port{ 5005 };
	α Start()ι->void;
	α Stop()ι->void;

	struct RequestHandler final : IRequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
	};
}
