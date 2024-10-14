#pragma once
#ifdef Jde_Web_Client_EXPORTS
	#ifdef _MSC_VER
		#define ΓWC __declspec( dllexport )
	#else
		#define ΓWC __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓWC __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Web.Client.lib")
		#else
			#pragma comment(lib, "Jde.Web.Client.lib")
		#endif
	#else
		#define ΓWC
	#endif
#endif