#pragma once
#include <jde/web/server/IRequestHandler.h>
#include <jde/web/Jwt.h>
#include <jde/web/server/Server.h>
#include "HttpRequestAwait.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }
namespace Jde::Web::Server{ struct RestStream; }
namespace Jde::App::Server{
	using namespace Jde::Web::Server;
	struct ServerSocketSession;
	α GetAppPK()ι->AppPK;
	α SetAppPKs( std::tuple<AppPK, AppInstancePK> x )ι->void;
	α QLPtr()ι->sp<QL::LocalQL>;
	α QL()ι->QL::LocalQL&;
	α SetLocalQL( sp<QL::LocalQL> ql )ι->void;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	α GetJwt( UserPK userPK, string name, string target, string endpoint, SessionPK sessionId, TimePoint expires, string description )ι->Web::Jwt;
	α RemoveExisting( str host, PortType port )ι->void;

	α StartWebServer( jobject&& settings )ε->void;
	α StopWebServer( bool terminate )ι->void;

	α BroadcastLogEntry( LogPK id, AppPK logAppPK, AppInstancePK logInstancePK, const Logging::Entry& m, const vector<string>& args )ι->void;
	α BroadcastStatus( AppPK appId, AppInstancePK statusInstancePK, str hostName, Proto::FromClient::Status&& status )ι->void;
	α BroadcastAppStatus()ι->void;
	α FindApplications( str name )ι->vector<Proto::FromClient::Instance>;
	α FindInstance( AppInstancePK instancePK )ι->sp<ServerSocketSession>;
	α NextRequestId()->RequestId;
	α RemoveSession( AppInstancePK sessionPK )ι->void;
	α SubscribeLogs( string&& qlText, jobject variables, sp<ServerSocketSession> session )ε->void;
	α SubscribeStatus( ServerSocketSession& session )ι->void;

	α UnsubscribeLogs( AppInstancePK instancePK )ι->bool;
	α UnsubscribeStatus( AppInstancePK instancePK )ι->bool;
	α Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε->void;

	struct RequestHandler final : IRequestHandler{
		RequestHandler( jobject&& settings )ι;
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override{ return Server::Schemas(); }
		α GetJwt( UserPK userPK, string&& name, string&& target, string&& endpoint, SessionPK sessionId, TimePoint expires, string&& description )ι->Web::Jwt;
	};
}