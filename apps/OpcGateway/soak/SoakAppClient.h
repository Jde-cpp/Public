#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Gateway::Soak{
	α AppClient()ι->sp<App::Client::IAppClient>;

	struct SoakAppClient final : App::Client::IAppClient{
		α ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SRCE )ε->up<TAwait<jvalue>> override;
	};
}
