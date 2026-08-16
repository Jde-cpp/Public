#pragma once

#ifdef NDEBUG
	#define BREAK
	#define BREAK_IF(x)
#else
	#ifdef _MSC_VER
		#define BREAK if( Logging::CanBreak() ){ DebugBreak(); }
	#else
		#define BREAK if( Logging::CanBreak() ){ ::raise( 5/*SIGTRAP*/ ); }
	#endif
	#define BREAK_IF(x) if( x ){ BREAK; }
#endif
#define Φ Γ auto
namespace Jde{
	Φ ToLogLevel( sv op )ι->ELogLevel;
}
namespace Jde::Logging{
	constexpr sv BreakTag{ "break" };//pseudo-tag in the log-settings mutations - sets this process's debugger-trap level.  Not an ELogTags: ToLogTags folds it into None, which is how "default" is spelled.
#ifndef NDEBUG
	Φ SetBreakLevel( ELogLevel level )ι->void;
	Φ BreakLevel()ι->ELogLevel;
	Φ CanBreak()ι->bool;
#endif
}
#undef Φ