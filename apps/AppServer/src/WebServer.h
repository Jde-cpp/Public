#pragma once
#include <jde/web/server/IRequestHandler.h>
#include <jde/web/Jwt.h>
#include <jde/web/server/Server.h>
#include "ql/QuerySessionsAwait.h"
#include "HttpRequestAwait.h"


namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }
namespace Jde::Web::Server{ struct RestStream; }
namespace Jde::App::Server{
	using namespace Jde::Web::Server;
	struct ServerSocketSession; struct RequestHandler;
	α GetAppPK()ι->ProgramPK;
	α SetAppPKs( std::tuple<ProgramPK, ProgInstPK, ConnectionPK> x )ι->void;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	α GetJwt( UserPK userPK, string name, string target, string endpoint, SessionPK sessionId, TimePoint expires, string description )ι->Web::Jwt;
	α RemoveExisting( str host, PortType port )ι->void;
	α GetRequestHandler()ι->sp<RequestHandler>;
	α StartWebServer( jobject&& settings )ε->void;
	α StopWebServer( bool terminate )ι->void;

	α BroadcastLogEntry( LogPK id, ProgramPK logAppPK, ProgInstPK logInstancePK, const Logging::Entry& m, const vector<string>& args )ι->void;
	//α BroadcastStatus( ProgramPK appId, ProgInstPK statusInstancePK, str hostName, Proto::FromClient::Status&& status )ι->void;
	α BroadcastAppStatus()ι->void;
	α FindApplications( str name )ι->vector<Proto::FromClient::Instance>;
	α FindConnection( ConnectionPK connectionPK )ι->sp<ServerSocketSession>;
	α NextRequestId()->RequestId;
	α RemoveSession( ProgInstPK sessionPK )ι->void;
	//α SubscribeLogs( string&& qlText, jobject variables, sp<ServerSocketSession> session )ε->void;
	//α SubscribeStatus( ServerSocketSession& session )ι->void;
	α QuerySessions( QL::TableQL ql, UserPK executer, SRCE )ι->QuerySessionsAwait;

	α UnsubscribeLogs( ProgInstPK instancePK )ι->bool;
	//α UnsubscribeStatus( ProgInstPK instancePK )ι->bool;
	α Write( ProgramPK appPK, optional<ProgInstPK> instancePK, Proto::FromServer::Transmission&& msg )ε->void;

	struct RequestHandler final : IRequestHandler{
		RequestHandler( jobject&& settings )ι;
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<HttpRequestAwait>( move(req), sl ); }
		α Jwt( UserPK userPK, string&& name, string&& target, string&& endpoint, SessionPK sessionId, TimePoint expires, string&& description )ι->Web::Jwt;
		α Query( QL::RequestQL&& ql, UserPK executer, bool raw, SRCE )ι->up<TAwait<jvalue>> override;
		α Schemas()ι->const vector<sp<DB::AppSchema>>& override{ return Server::Schemas(); }
		α WebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession> override;
	};
}