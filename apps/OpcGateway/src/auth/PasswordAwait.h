#pragma once
#include <jde/app/client/AppClientSocketSession.h>
#include "AuthAwait.h"

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct PasswordAwait : AuthAwait<Web::FromServer::SessionInfo>{
		PasswordAwait( string loginName, string password, string opcNK, string endpoint, bool isSocket, SRCE )ι;
	private:
		α OnSuccess()ι->void{ CheckProvider(); }
		α CheckProvider()ι->TAwait<Access::ProviderPK>::Task;
		α AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task;
	};
}