#pragma once
#ifdef Jde_Web_Client_EXPORTS
	#ifdef _MSC_VER
		#define Jde_Web_Client_EXPORT __declspec( dllexport )
		#define ΓWC Jde_Web_Client_EXPORT
	#else
		#define Jde_Web_Client_EXPORT __attribute__((visibility("default")))
		#define ΓWC Jde_Web_Client_EXPORT
	#endif
#else
	#ifdef _MSC_VER
		#define Jde_Web_Client_EXPORT __declspec( dllimport )
		#define ΓWC Jde_Web_Client_EXPORT
		#if NDEBUG
			#pragma comment(lib, "Jde.Web.Client.lib")
		#else
			#pragma comment(lib, "Jde.Web.Client.lib")
		#endif
	#else
		#define Jde_Web_Client_EXPORT
		#define ΓWC
	#endif
#endif