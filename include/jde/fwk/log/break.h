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
namespace Jde::Logging{
	Φ SetBreakLevel()ι->void;
	Φ BreakLevel()ι->ELogLevel;
	Φ CanBreak()ι->bool;
}
#undef Φ