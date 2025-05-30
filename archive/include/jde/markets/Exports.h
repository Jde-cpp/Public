#pragma once

#ifdef JDE_EXPORT_MARKETS
	#ifdef _MSC_VER
		#define ΓM __declspec( dllexport )
	#else
		#define ΓM __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓM __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Markets.lib")
		#else
			#pragma comment(lib, "Jde.Markets.lib")
		#endif
	#else
		#define ΓM
	#endif
#endif
