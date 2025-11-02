#pragma once
#include <jde/fwk/co/Await.h>

namespace Jde::App::Server{
	α InitLogging()ι->void;
	struct AppStartupAwait final : VoidAwait{
		AppStartupAwait( jobject webServerSettings, SRCE )ε:VoidAwait{sl},_webServerSettings{move(webServerSettings)}{}
	private:
		α Suspend()ι->void{ Execute(); }
		α Execute()ι->VoidAwait::Task;
		jobject _webServerSettings;
	};
}