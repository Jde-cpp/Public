#pragma once

#ifdef JDE_EXPORT_MARKETS
	#ifdef _MSC_VER
		#define JDE_MARKETS_EXPORT __declspec( dllexport )
	#else
		#define JDE_MARKETS_EXPORT __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define JDE_MARKETS_EXPORT __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Markets.lib")
		#else
			#pragma comment(lib, "Jde.Markets.lib")
		#endif
	#else
		#define JDE_MARKETS_EXPORT
	#endif
#endif
