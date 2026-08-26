#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Emulator{
	α AppClient()ι->sp<App::Client::IAppClient>;

	//The emulator's AppServer client: logs in (the session id becomes the OPC UA issued token) and answers `status` - nothing else is queried of it.
	struct EmulatorAppClient final : App::Client::IAppClient{
		α ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SRCE )ε->up<TAwait<jvalue>> override;
	};
}
