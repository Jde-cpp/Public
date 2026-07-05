#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Gateway{
	α AppClient()ι->sp<App::Client::IAppClient>;

	struct GatewayAppClient final : App::Client::IAppClient{
		α ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ε->up<TAwait<jvalue>> override;
	};
}