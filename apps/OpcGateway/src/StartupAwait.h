#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::App::Client{ struct IAppClient; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }
namespace Jde::Opc::Gateway{
	α QL()ι->QL::LocalQL&;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	α AppClient()ι->sp<App::Client::IAppClient>;
	struct StartupAwait final : VoidAwait<>{
		StartupAwait( jobject webServerSettings, jobject userName, SRCE )ι:VoidAwait<>{sl},_webServerSettings{move(webServerSettings)}, _userName{move(userName)}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->VoidAwait<>::Task;

		jobject _webServerSettings;
		jobject _userName;
	};
}