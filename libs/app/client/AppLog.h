#pragma once
#include <jde/framework/settings.h>

namespace Jde::App::Client{
	struct AppLog final : Logging::ILogger{
		AppLog()ι:ILogger{ Settings::FindDefaultObject("/logging/app") }{};
		α Name()Ι->string override{ return "proto"; }
		α Write( const Logging::Entry& m )ι->void override;
	};
}