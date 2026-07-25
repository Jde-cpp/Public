#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Gateway::Soak{
	α Run( sp<App::Client::IAppClient> client )ε->int;//blocks for the configured duration; returns the process exit code.
}
