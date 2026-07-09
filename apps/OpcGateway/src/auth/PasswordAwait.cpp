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
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α PasswordAwait::AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			auto sessionInfo = co_await AppClient()->AddSession( _opcNK, _cred.LoginName(), providerPK, _endpoint, false );
			Gateway::AddSession( sessionInfo.session_id(), _opcNK, move(_cred) );
			Resume( move(sessionInfo) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α PasswordAwait::await_resume()ε->optional<Web::FromServer::SessionInfo>{
		if( Promise() )
			return base::await_resume();
		//Cache hit (await_ready()==true): AuthCache already registered _sessionId as authenticated for this opc, but Execute()/AddSession never ran so there's no SessionInfo. Return one carrying that session id instead of nullopt, otherwise Login skips setting the session id and the client gets none. (The deeper "a fresh login shouldn't reuse another session's cache" concern is the AuthCache design, review #14.)
		Web::FromServer::SessionInfo info;
		info.set_session_id( _sessionId );
		return info;
	}
}