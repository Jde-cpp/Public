#pragma once
#include <jde/framework/coroutine/Await.h>


namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }
namespace Jde::Opc::Gateway{
	α QL()ι->QL::LocalQL&;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	struct StartupAwait final : VoidAwait<>{
		StartupAwait( jobject webServerSettings, SRCE )ι:VoidAwait<>{sl},_webServerSettings{move(webServerSettings)}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->VoidAwait<>::Task;

		jobject _webServerSettings;
	};
}
