#pragma once
#ifdef Jde_HTTP_EXPORTS
	#ifdef _MSC_VER
		#define ΓH __declspec( dllexport )
	#else
		#define ΓH __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓH __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Http.lib")
		#else
			#pragma comment(lib, "Jde.Http.lib")
		#endif
	#else
		#define ΓH
	#endif
#endif