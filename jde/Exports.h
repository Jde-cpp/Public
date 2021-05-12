#pragma once
#ifdef Jde_EXPORTS
	#ifdef _MSC_VER 
		#define JDE_NATIVE_VISIBILITY __declspec( dllexport )
	#else
		#define JDE_NATIVE_VISIBILITY __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define JDE_NATIVE_VISIBILITY __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.lib")
		#else
			#pragma comment(lib, "Jde.lib")
		#endif
	#else
		#define JDE_NATIVE_VISIBILITY
	#endif
#endif