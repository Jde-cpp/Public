#pragma once
#include <jde/web/server/IRequestHandler.h>

namespace Jde::App{ struct IApp; }
namespace Jde::Web::Server{ struct HttpRequest; struct IHttpRequestAwait; struct IWebsocketSession;struct RestStream; }
namespace Jde::Opc::Gateway{
	α StartWebServer( jobject&& settings )ε->void;
	α StopWebServer( bool terminate )ι->void;
	namespace Server{
		α RemoveSession( uint socketSessionId )ι->void;
	}
	struct RequestHandler final : Web::Server::IRequestHandler{
		RequestHandler( jobject settings, sp<App::IApp> appClient, sp<QL::IQL> ql )ι:Web::Server::IRequestHandler{move(settings), move(appClient)}, _ql{move(ql)}{}
		α HandleRequest( Web::Server::HttpRequest&& req, SRCE )ι->up<Web::Server::IHttpRequestAwait> override;
		α WebsocketSession( sp<Web::Server::RestStream>&& stream, beast::flat_buffer&& buffer, Web::Server::TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<Web::Server::IWebsocketSession> override;
		α QLServer()ι->sp<QL::IQL> override{ return _ql; }
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override;
	private:
		sp<QL::IQL> _ql;
	};
}