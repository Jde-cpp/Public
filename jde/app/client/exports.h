#pragma once
#ifdef Jde_App_Client_EXPORTS
	#ifdef _MSC_VER
		#define ΓAC __declspec( dllexport )
	#else
		#define ΓAC __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓAC __declspec( dllimport )
		#pragma comment(lib, "Jde.App.Client.lib")
	#else
		#define ΓAC
	#endif
#endif