#pragma once
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/exports.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/web/client/Jwt.h>

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; struct Authorize;}
namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::App::Client{
	α UpdateStatus()ι->void;
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;
	α AppServiceUserPK()ι->UserPK;	//for internal queries.

	α IsSsl()ι->bool;
	α Host()ι->string;
	α Port()ι->PortType;
	α RemoteAcl( string libName )ι->sp<Access::Authorize>;
	α QLServer()ε->sp<QL::IQL>;

	struct ΓAC ConnectAwait final : VoidAwait<>{
		ConnectAwait( vector<sp<DB::AppSchema>>&& subscriptionSchemas, bool retry=false, SRCE )ι;
	private:
		α Suspend()ι->void{ HttpLogin(); }
		α HttpLogin()ι->TAwait<SessionPK>::Task;
		α RunSocket( SessionPK sessionId )ι->TAwait<Proto::FromServer::ConnectionInfo>::Task;
		α Retry()->VoidAwait<>::Task;
		bool _retry{false};
		vector<sp<DB::AppSchema>> _subscriptionSchemas;
	};
	Ξ Connect(vector<sp<DB::AppSchema>>&& subscriptionSchemas)ι->ConnectAwait::Task{ co_await ConnectAwait{move(subscriptionSchemas)}; }

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