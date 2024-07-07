#pragma once
#include <jde/Log.h>

namespace Jde::App::Client{
	struct AppClientSocketSession;
	struct AppLog final : Logging::IExternalLogger{
		//using namespace Jde::Logging;
		AppLog()ι{};
		α Destroy(SRCE)ι->void override;
		α Name()ι->string override{ return "db"; }
		α Log( Logging::ExternalMessage&& m, SRCE )ι->void override;
		α Log( const Logging::ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void override;
		α SetMinLevel(ELogLevel level)ι->void override;
	};
}