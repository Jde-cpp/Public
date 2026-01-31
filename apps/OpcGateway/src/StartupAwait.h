#pragma once
#include <jde/fwk/co/Await.h>

namespace Jde::App::Client{ struct IAppClient; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }
namespace Jde::Opc::Gateway{
	α AppClient()ι->sp<App::Client::IAppClient>;
	struct StartupAwait final : VoidAwait{
		StartupAwait( jobject webServerSettings, jobject userName, SRCE )ι;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->VoidAwait::Task;

		jobject _webServerSettings;
		jobject _userName;
	};
}