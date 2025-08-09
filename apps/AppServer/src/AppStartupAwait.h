#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::App::Server{
	struct AppStartupAwait final : VoidAwait{
		AppStartupAwait( jobject webServerSettings, SRCE )ε:VoidAwait{sl},_webServerSettings{move(webServerSettings)}{}
	private:
		α Suspend()ι->void{ Execute(); }
		α Execute()ι->VoidAwait::Task;
		jobject _webServerSettings;
	};
}