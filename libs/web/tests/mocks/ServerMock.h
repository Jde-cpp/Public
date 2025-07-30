#pragma once
#include "HttpRequestAwait.h"
#include <jde/web/server/IRequestHandler.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Server;
	const string Host{ "localhost" };
	constexpr PortType Port{ 5005 };
	α Start( jobject settings )ε->void;
	α Stop()ι->void;
	α AppClient()ι->sp<App::IApp>;

	struct RequestHandler final : IRequestHandler{
		RequestHandler( jobject settings )ι: IRequestHandler{ settings, AppClient() }{}
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>&{ return _schemas; }
	private:
		vector<sp<DB::AppSchema>> _schemas;
	};
}
