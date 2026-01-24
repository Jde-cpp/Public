#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/web/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/proto/App.FromServer.pb.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/exports.h>
#include <jde/fwk/crypto/OpenSsl.h>

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; struct Authorize;}
namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::App::Client{
	struct IAppClient;
	α IsSsl()ι->bool;
	α Host()ι->string;
	α Port()ι->PortType;
	α RemoteAcl( string libName )ι->sp<Access::Authorize>;
	α SetAcl( sp<Access::Authorize> acl )ι->void;

	struct ConnectAwait final : VoidAwait{
		ConnectAwait( sp<IAppClient> appClient, bool retry, SRCE )ι;
	private:
		α Suspend()ι->void{ HttpLogin(); }
		α HttpLogin()ι->TAwait<SessionPK>::Task;
		α RunSocket( SessionPK sessionId )ι->TAwait<Proto::FromServer::ConnectionInfo>::Task;
		α Retry()->DurationTimer::Task;

		sp<IAppClient> _appClient;
		bool _retry;
	};
	α Connect( sp<IAppClient>&& appClient )ι->ConnectAwait::Task;
}