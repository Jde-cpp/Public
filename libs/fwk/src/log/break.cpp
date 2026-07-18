#include <jde/fwk/log/break.h>

namespace Jde{
	ELogLevel _breakLevel = ELogLevel::Warning;
	α Logging::SetBreakLevel()ι->void{
		_breakLevel = Settings::FindEnum<ELogLevel>( "/logging/breakLevel", ToLogLevel ).value_or( ELogLevel::Warning );
	}
	α Logging::BreakLevel()ι->ELogLevel{ return _breakLevel; }

	α Logging::CanBreak()ι->bool{ return Process::IsDebuggerPresent(); }
}