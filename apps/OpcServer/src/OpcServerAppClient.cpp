#include "OpcServerAppClient.h"
#include <jde/app/client/appClient.h>
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/ql/IQL.h>

namespace Jde::Opc::Server{
	α OpcServerAppClient::StatusDetails()ι->vector<string>{
		return App::Client::IAppClient::CommonDetails();
	}
}