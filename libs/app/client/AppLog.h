#pragma once
#include <jde/log/Log.h>

namespace Jde::App::Client{
	struct AppLog final : Logging::IExternalLogger{
		AppLog()ι{};
		α Destroy(SRCE)ι->void override;
		α Name()ι->string override{ return "db"; }
		α Log( Logging::ExternalMessage&& m, SRCE )ι->void override{ Log( m, nullptr, sl ); }
		α Log( const Logging::ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void override;
		α SetMinLevel(ELogLevel level)ι->void override;
	};
}