#pragma once
#include <jde/app/client/usings.h>
#include <jde/app/client/AppClientSubscriptions.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include <jde/access/usings.h>
#include <jde/ql/types/Subscription.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include "exports.h"

#define Φ ΓAC auto
namespace Jde::QL{ struct IListener; }
namespace Jde::App::Client{
	struct StartSocketAwait : TAwait<Proto::FromServer::ConnectionInfo>{
		using base = TAwait<Proto::FromServer::ConnectionInfo>;
		StartSocketAwait( SessionPK sessionId, SRCE )ι;
	private:
		α Suspend()ι->void override;
		α RunSession()ι->VoidTask;
		α SendSessionId()ι->Web::Client::ClientSocketAwait<Proto::FromServer::ConnectionInfo>::Task;
		SessionPK _sessionId;
	};
	Φ AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, SRCE )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>;
	α CloseSocketSession( SRCE )ι->VoidTask;
	//Φ Query( str query, SRCE )ε->Web::Client::ClientSocketAwait<string>;
	//Φ Subscribe( string&& query, sp<QL::IListener> listener, QL::SubscriptionClientId clientId, SRCE )ε->ClientSubscriptionAwait;
	//Φ Unsubscribe( str query, SRCE )ε->UnsubscribeAwait;

	struct AppClientSocketSession final : Web::Client::TClientSocketSession<Jde::App::Proto::FromClient::Transmission,Jde::App::Proto::FromServer::Transmission>{
		Τ using await = Web::Client::ClientSocketAwait<T>;
		using base = Web::Client::TClientSocketSession<Proto::FromClient::Transmission,Proto::FromServer::Transmission>;
		ΓAC Ω Instance()ι->sp<AppClientSocketSession>;
		AppClientSocketSession( sp<net::io_context> ioc, optional<ssl::context> ctx )ι;
		α Connect( SessionPK sessionId, SRCE )ι->await<Proto::FromServer::ConnectionInfo>;
		α SessionInfo( SessionPK sessionId, SRCE )ι->await<Web::FromServer::SessionInfo>;
		α Query( string&& q, bool returnRaw, SRCE )ι->await<jvalue> override;
		α Subscribe( string&& query, sp<QL::IListener> listener, SRCE )ε->await<jarray> override;
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
		Jde::UserPK _userPK{};
		sp<QL::IQL> _qlServer;
	};
}
#undef Φ