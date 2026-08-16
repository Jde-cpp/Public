#include <jde/fwk/log/break.h>

#ifndef NDEBUG
namespace Jde{
	atomic<ELogLevel> _breakLevel = ELogLevel::Warning;
	α Logging::SetBreakLevel( ELogLevel level )ι->void{
		_breakLevel = level;
	}
	α Logging::BreakLevel()ι->ELogLevel{ return _breakLevel.load(); }

	α Logging::CanBreak()ι->bool{ return Process::IsDebuggerPresent(); }
}
#endif