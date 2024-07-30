#include <jde/app/client/AppClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/usings.h>

#define var const auto

namespace Jde{
	//UserPK _appServiceUserPK{0};
}

namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	//using Web::Client::ClientHttpRes;

	//α Client::AppServiceUserPK()ι->UserPK{ return _appServiceUserPK; }
//	α Client::SetAppServiceUserPK(UserPK x)ι->void{ _appServiceUserPK = x; }

	function<vector<string>()> _statusDetails = []->vector<string>{ return {}; };
  α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	#define IF_OK if( auto pSession = Process::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	// update immediately
	α Client::UpdateStatus()ι->void{
		IF_OK
			pSession->Write( FromClient::Status(_statusDetails()) );
	}
namespace Client{
	LoginAwait::LoginAwait( Crypto::Modulus mod, Crypto::Exponent exp, string userName, string userTarget, string myEndpoint, string description, const fs::path& privateKeyPath, SL sl )ι:
		base{sl},
		_jwt{ move(mod), move(exp), move(userName), move(userTarget), move(myEndpoint), move(description), privateKeyPath }
	{}
	α LoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			json j{ j, _jwt.Payload() };
			auto res = co_await ClientHttpAwait{ Settings::Get("server/host").value_or("localhost"), "/CertificateLogin", j, Settings::Get<PortType>("server/port").value_or(1967) };
			auto sessionPK = Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			THROW_IF( !sessionPK, "Invalid authorization: {}.", res[http::field::authorization] );
			Resume( move(*sessionPK) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}}