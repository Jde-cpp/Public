#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Gateway{
	α AppClient()ι->sp<App::Client::IAppClient>;

	struct GatewayAppClient final : App::Client::IAppClient{
		α StatusDetails()ι->vector<string> override;
	};
}