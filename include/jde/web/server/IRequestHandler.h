#pragma once
#include "usings.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Web::Server{
	struct HttpRequest; struct IHttpRequestAwait; struct IWebsocketSession;struct RestStream;
	struct IRequestHandler{
		virtual ~IRequestHandler()=default; //msvc error
		β HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> =0; //abstract, can't return a copy.
		β GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> =0;
		β Schemas()ι->const vector<sp<DB::AppSchema>>& =0;
	};
}