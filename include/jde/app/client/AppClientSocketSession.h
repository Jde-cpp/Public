#pragma once
#include <jde/app/client/usings.h>
#include <jde/app/client/AppClientSubscriptions.h>
#include <jde/app/proto/app.FromClient.h>
#include <jde/access/usings.h>
#include <jde/ql/types/Subscription.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include "../proto/App.FromServer.pb.h"
#include "exports.h"

#define Φ ΓAC auto
namespace Jde::QL{ struct IListener; }
namespace Jde::App::Client{
	struct IAppClient;
	struct StartSocketAwait : TAwait<Proto::FromServer::ConnectionInfo>{
		using base = TAwait<Proto::FromServer::ConnectionInfo>;
		StartSocketAwait( SessionPK sessionId, sp<Access::Authorize> authorize, sp<IAppClient> appClient, SL sl )ι;
	private:
		α Suspend()ι->void override;
		α RunSession()ι->VoidTask;
		α SendSessionId()ι->Web::Client::ClientSocketAwait<Proto::FromServer::ConnectionInfo>::Task;
		sp<IAppClient> _appClient;
		sp<Access::Authorize> _authorize;
		SessionPK _sessionId;
		sp<Client::AppClientSocketSession> _session;
	};
	α CloseSocketSession( SRCE )ι->VoidTask;

	struct AppClientSocketSession final : Web::Client::TClientSocketSession<Jde::App::Proto::FromClient::Transmission,Jde::App::Proto::FromServer::Transmission>{
		Τ using await = Web::Client::ClientSocketAwait<T>;
		using base = Web::Client::TClientSocketSession<Proto::FromClient::Transmission,Proto::FromServer::Transmission>;
		AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx, sp<Access::Authorize> authorize, sp<IAppClient> appClient )ι;
		α Connect( SessionPK sessionId, SRCE )ι->await<Proto::FromServer::ConnectionInfo>;
		α SessionInfo( SessionPK creds, SRCE )ι->await<Web::FromServer::SessionInfo>;
		α Query( string&& q, jobject variables, bool returnRaw, SRCE )ι->await<jvalue> override;
		α Subscribe( string&& query, jobject variables, sp<QL::IListener> listener, SRCE )ε->await<jarray>;
		α Unsubscribe( string&& query, SRCE )ε->await<vector<QL::SubscriptionId>>;
		α UserPK()Ι{ return _userPK; }
		α QLServer()ι{ return _qlServer; }
	private:
		α Execute( string&& bytes, optional<Jde::UserPK> userPK, RequestId clientRequestId )ι->void;
		α WriteException( exception&&, RequestId requestId )ι->void;
		α WriteException( string&& e, RequestId requestId )ι->void;
		α ProcessTransmission( Proto::FromServer::Transmission&& t, optional<Jde::UserPK> userPK, optional<RequestId> clientRequestId )ι->void;
		α HandleException( std::any&& h, IException&& what, RequestId requestId )ι->void;
		α OnRead( Proto::FromServer::Transmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		α OnMessage( string&& j, RequestId requestId )ι->void;
		sp<IAppClient> _appClient;
		sp<Access::Authorize> _authorize;
		sp<QL::IQL> _qlServer;
		optional<Proto::FromServer::ConnectionInfo> _sessionInfo;
		Jde::UserPK _userPK{};
	};
}
#undef Φ