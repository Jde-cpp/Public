#pragma once
#ifdef Jde_EXPORTS
	#ifdef _MSC_VER
		#define Γ __declspec( dllexport )
	#else
		#define Γ __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define Γ __declspec( dllimport )
		// #if NDEBUG
		// 	#pragma comment(lib, "Jde.lib")
		// #else
		// 	#pragma comment(lib, "Jde.lib")
		// #endif
	#else
		#define Γ
	#endif
#endif