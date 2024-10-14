#pragma once
#ifdef Jde_EXPORTS
	#ifdef _MSC_VER
		#define ΓDB __declspec( dllexport )
	#else
		#define ΓDB __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓDB __declspec( dllimport )
		// #if NDEBUG
		// 	#pragma comment(lib, "Jde.lib")
		// #else
		// 	#pragma comment(lib, "Jde.lib")
		// #endif
	#else
		#define ΓDB
	#endif
#endif