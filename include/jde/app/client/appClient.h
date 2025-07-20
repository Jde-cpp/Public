#pragma once
#include <jde/web/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/exports.h>
#include <jde/crypto/OpenSsl.h>

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; struct Authorize;}
namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::App::Client{
	struct IAppClient;
	α IsSsl()ι->bool;
	α Host()ι->string;
	α Port()ι->PortType;
	α RemoteAcl( string libName )ι->sp<Access::Authorize>;

	struct ConnectAwait final : VoidAwait<>{
		ConnectAwait( sp<IAppClient> appClient, jobject userName, bool retry=false, SRCE )ι;
	private:
		α Suspend()ι->void{ HttpLogin(); }
		α HttpLogin()ι->TAwait<SessionPK>::Task;
		α RunSocket( SessionPK sessionId )ι->TAwait<Proto::FromServer::ConnectionInfo>::Task;
		α Retry()->VoidAwait<>::Task;

		sp<IAppClient> _appClient;
		bool _retry;
		jobject _userName;
	};
	Ξ Connect( sp<IAppClient>&& appClient )ι->ConnectAwait::Task{
		co_await ConnectAwait{ move(appClient), true};
	}

/*
	//TODO change to functions, not returning anything.
	struct LogAwait : VoidAwait<>{
		using base = VoidAwait<>;
		LogAwait( Logging::ExternalMessage&& m, SRCE )ι:base{sl}, _message{move(m)}{}
		LogAwait( const Logging::ExternalMessage& m, const vector<string>* args, SRCE )ι:base{sl}, _message{ m }, _args{ args }{}
		const Logging::ExternalMessage _message;
		const vector<string>* _args;
	};
*/
}