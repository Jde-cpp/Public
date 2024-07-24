#pragma once
#ifdef Jde_APP_SHARED_EXPORTS
	#ifdef _MSC_VER
		#define ΓAS __declspec( dllexport )
	#else
		#define ΓAS __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓAS __declspec( dllimport )
		#pragma comment(lib, "Jde.App.Shared.lib")
	#else
		#define ΓAS
	#endif
#endif