#pragma once
#include <jde/app/client/IAppClient.h>
#include <jde/fwk/co/Await.h>
#include <jde/web/server/Sessions.h>

namespace Jde::Opc::Server{
	struct OpcServerAppClient final : App::Client::IAppClient{
		α StatusDetails()ι->vector<string> override;
		α InitLogging()ι->void;
	};
}