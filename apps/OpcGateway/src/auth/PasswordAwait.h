#pragma once
#include <jde/app/client/AppClientSocketSession.h>
#include "AuthAwait.h"

namespace Jde::Opc::Gateway{
	struct UAClient;

	struct PasswordAwait : AuthAwait<optional<Web::FromServer::SessionInfo>>{// nullopt=use current session.
		PasswordAwait( string loginName, string password, string opcNK, string endpoint, bool isSocket, SessionPK sessionId, SRCE )ι;
		α await_resume()ι->optional<Web::FromServer::SessionInfo>;
	private:
		α OnSuccess()ι->void{ CheckProvider(); }
		α CheckProvider()ι->TAwait<Access::ProviderPK>::Task;
		α AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task;
	};
}