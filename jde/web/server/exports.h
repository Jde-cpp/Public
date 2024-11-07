#pragma once

#ifdef Jde_Web_Server_EXPORTS
	#ifdef _MSC_VER
		#define ΓWS __declspec( dllexport )
	#else
		#define ΓWS __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓWS __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Web.Server.lib")
		#else
			#pragma comment(lib, "Jde.Web.Server.lib")
		#endif
	#else
		#define ΓWS
	#endif
#endif