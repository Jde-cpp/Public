#include "GatewayAppClient.h"

namespace Jde::Opc{
	static sp<App::Client::IAppClient> _appClient = ms<Gateway::GatewayAppClient>();
	α Gateway::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }
}
namespace Jde::Opc::Gateway{
	α GatewayAppClient::StatusDetails()ι->vector<string>{
		return IAppClient::CommonDetails();
	}
}