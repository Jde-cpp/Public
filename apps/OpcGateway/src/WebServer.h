#pragma once
#include <jde/web/server/IRequestHandler.h>
#include <jde/web/server/IApplicationServer.h>

namespace Jde::Web::Server{ struct HttpRequest; struct IHttpRequestAwait; struct IWebsocketSession;struct RestStream; }
namespace Jde::Opc{
	α StartWebServer( jobject&& settings )ε->void;
	α StopWebServer()ι->void;
	namespace Server{
		α RemoveSession( uint socketSessionId )ι->void;
	}
	struct RequestHandler final : Web::Server::IRequestHandler{
		α HandleRequest( Web::Server::HttpRequest&& req, SRCE )ι->up<Web::Server::IHttpRequestAwait> override;
		α GetWebsocketSession( sp<Web::Server::RestStream>&& stream, beast::flat_buffer&& buffer, Web::Server::TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<Web::Server::IWebsocketSession> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override;
	};
}