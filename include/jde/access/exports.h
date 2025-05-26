#pragma once
#ifdef Jde_ACCESS_EXPORTS
	#ifdef _MSC_VER
		#define ΓA __declspec( dllexport )
	#else
		#define ΓA __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓA __declspec( dllimport )
	#else
		#define ΓA
	#endif
#endif