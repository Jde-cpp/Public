#pragma once

namespace Jde::DB{ struct AppSchema; }

namespace Jde::Opc::Server{
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	struct StartupAwait final : VoidAwait{
		StartupAwait( jobject webServerSettings, jobject userName, SRCE )ι:VoidAwait{sl},_webServerSettings{move(webServerSettings)}, _userName{move(userName)}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->VoidAwait::Task;

		jobject _webServerSettings;
		jobject _userName;
	};
}