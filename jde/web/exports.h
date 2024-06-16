#pragma once
#ifdef Jde_WEB_EXPORTS
	#ifdef _MSC_VER
		#define ΓW __declspec( dllexport )
	#else
		#define ΓW __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓW __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Web.lib")
		#else
			#pragma comment(lib, "Jde.Web.lib")
		#endif
	#else
		#define ΓW
	#endif
#endif