#pragma once
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/exports.h>
//#include <jde/access/IAcl.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/web/client/Jwt.h>

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{ struct IDataSource; }
namespace Jde::App::Client{
	α UpdateStatus()ι->void;
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;
	α AppServiceUserPK()ι->UserPK;	//for internal queries.
	ΓAC α Connect( bool wait=false )ι->void;

	α IsSsl()ι->bool;
	α Host()ι->string;
	α Port()ι->PortType;
	α RemoteAcl()ι->sp<Access::IAcl>;
	α QLServer()ε->sp<QL::IQL>;

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