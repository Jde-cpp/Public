#pragma once
#pragma once
//#include <jde/web/flex/HttpRequestAwait.h>
//#include <jde/web/flex/Flex.h>
#include "HttpRequestAwait.h"

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;
	constexpr sv Host{ "localhost" };
	constexpr sv Port{ "5005" };
	α Start()ι->void;
	α Stop()ι->void;

	struct RequestHandler final : IRequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α RunWebsocketSession( RestStream&& stream, beast::flat_buffer&& buffer, TRequestType req )ι->void override;
	};
}
