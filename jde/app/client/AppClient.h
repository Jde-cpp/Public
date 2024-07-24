#pragma once
//#include <jde/app/client/Sessions.h>
#include <jde/app/client/usings.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/web/client/Jwt.h>

namespace Jde::DB{ struct IDataSource; }
namespace Jde::App::Client{
	α UpdateStatus()ι->void;
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;
	α AppServiceUserPK()ι->UserPK;	//for internal queries.

	struct LoginAwait final : TAwait<SessionPK>{
		using base = TAwait<SessionPK>;
		LoginAwait( Crypto::Modulus mod, Crypto::Exponent exp, string userName, string userTarget, string myEndpoint, string description, SRCE )ι;
		α await_suspend( base::Handle h )ι->void{ base::await_suspend(h); Execute(); };
	private:
		α Execute()ι->void;
		Web::Jwt _jwt;
		//Crypto::Modulus _modulus; Crypto::Exponent _exponent; string _userName; string _userTarget; string _myEndpoint; string _description;
	};
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