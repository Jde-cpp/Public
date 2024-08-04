#pragma once
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/client/usings.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/web/client/Jwt.h>

namespace Jde::DB{ struct IDataSource; }
namespace Jde::App::Client{
	α UpdateStatus()ι->void;
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;
	α AppServiceUserPK()ι->UserPK;	//for internal queries.
	α Connect( bool wait=false, SRCE )ι->void;

	α IsSsl()ι->bool;
	α Host()ι->string;
	α Port()ι->PortType;

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