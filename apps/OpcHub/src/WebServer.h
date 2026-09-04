#pragma once
#include "../../AppServer/src/WebServer.h"

namespace Jde::Opc::Hub{
	//One listener for both roles.  An App::Server::RequestHandler (GetJwt signs through it, its base holds the LocalClient the
	//session code asks IsLocal of) whose /graphql is the HubQL over access+app+gateway and whose websocket upgrades go by path:
	//`/` (or none) is the app protocol - the SPA's AppService, the OpcServer, the PLC emulator and remote gateways all register
	//there unchanged - and `/opc` the gateway's.  The path, not Sec-WebSocket-Protocol: the protocol is fixed before the first
	//frame (each session's Ack), beast negotiates no subprotocol, and the standalone gateway ignores the path anyway.
	constexpr sv OpcSocketPath{ "/opc" };
	struct RequestHandler final : App::Server::RequestHandler{
		RequestHandler( jobject&& settings )ι: App::Server::RequestHandler{ move(settings) }{}
		α HandleRequest( Web::Server::HttpRequest&& req, SRCE )ι->up<Web::Server::IHttpRequestAwait> override;
		α WebsocketSession( sp<Web::Server::IRestStream>&& stream, beast::flat_buffer&& buffer, Web::Server::TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<Web::Server::IWebsocketSession> override;
	};
}
