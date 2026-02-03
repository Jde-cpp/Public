#include "PasswordAwait.h"
#include <jde/app/client/IAppClient.h>
#include "../StartupAwait.h"
#include "../UAClient.h"
#include "UM.h"

#define let const auto

namespace Jde::Opc::Gateway{
	PasswordAwait::PasswordAwait( string loginName, string password, string opcNK, string endpoint, bool isSocket, SessionPK sessionId, SL sl )ι:
		AuthAwait{ { {move(loginName), move(password)} }, move(opcNK), move(endpoint), isSocket, sessionId, sl }{}

	α PasswordAwait::CheckProvider()ι->TAwait<Access::ProviderPK>::Task{
		try{
			let providerPK = co_await ProviderAwait{ _opcNK };
			THROW_IF( providerPK==0, "Provider not found for '{}'.", _opcNK );
			AddSession( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α PasswordAwait::AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			auto sessionInfo = co_await AppClient()->AddSession( _opcNK, _cred.LoginName(), providerPK, _endpoint, false );
			Gateway::AddSession( sessionInfo.session_id(), _opcNK, move(_cred) );
			Resume( move(sessionInfo) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α PasswordAwait::await_resume()ε->optional<Web::FromServer::SessionInfo>{
		return Promise() ? base::await_resume() : nullopt;
	}
}