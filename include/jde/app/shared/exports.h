#pragma once
#ifdef Jde_App_Shared_EXPORTS
	#ifdef _MSC_VER
		#define ΓAS __declspec( dllexport )
		#define Jde_App_Shared_EXPORT __declspec( dllexport )
	#else
		#define ΓAS __attribute__((visibility("default")))
		#define Jde_App_Shared_EXPORT __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓAS __declspec( dllimport )
		#define Jde_App_Shared_EXPORT __declspec( dllimport )
		#pragma comment(lib, "Jde.App.Shared.lib")
	#else
		#define ΓAS
		#define Jde_App_Shared_EXPORT
	#endif
#endif