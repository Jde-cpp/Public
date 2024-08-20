#pragma once
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include <jde/app/client/usings.h>
#include <jde/app/shared/proto/App.FromClient.h>

namespace Jde::App::Client{
	struct StartSocketAwait : VoidAwait<>{
		using base = VoidAwait<>;
		StartSocketAwait( SessionPK sessionId, SRCE )ι;
		α Suspend()ι->void override;
		SessionPK _sessionId;
	};
	α AddSession( str domain, str loginName, ProviderPK providerPK, str userEndPoint, bool isSocket, SRCE )ι->Web::Client::ClientSocketAwait<Proto::FromServer::SessionInfo>;
	α CloseSocketSession( SRCE )ι->VoidTask;
	α GraphQL( str query, SRCE )ε->Web::Client::ClientSocketAwait<string>;

	struct AppClientSocketSession final : Web::Client::TClientSocketSession<Jde::App::Proto::FromClient::Transmission,Jde::App::Proto::FromServer::Transmission>{
		using base = Web::Client::TClientSocketSession<Proto::FromClient::Transmission,Proto::FromServer::Transmission>;
		Ω Instance()ι->sp<AppClientSocketSession>;
		AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι;
		α Connect( SessionPK sessionId, SRCE )ι->Web::Client::ClientSocketAwait<Proto::FromServer::ConnectionInfo>;
		α SessionInfo( SessionPK sessionId, SRCE )ι->Web::Client::ClientSocketAwait<Proto::FromServer::SessionInfo>;
		α GraphQL( string&& q, UserPK userPK, SRCE )ι->Web::Client::ClientSocketAwait<string>;

	private:
		α Execute( string&& bytes, optional<UserPK> userPK, RequestId clientRequestId )ι->void;
		α WriteException( IException&&, RequestId requestId )->void;
		α ProcessTransmission( Proto::FromServer::Transmission&& t, optional<UserPK> userPK, optional<RequestId> clientRequestId )ι->void;
		α HandleException( std::any&& h, IException&& what, RequestId requestId )ι->void;
		α OnRead( Proto::FromServer::Transmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		UserPK _userPK{};//TODO
	};
}