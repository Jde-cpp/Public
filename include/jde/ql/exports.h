#pragma once
#ifdef Jde_QL_EXPORTS
	#ifdef _MSC_VER
		#define ΓQL __declspec( dllexport )
	#else
		#define ΓQL __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓQL __declspec( dllimport )
		#pragma comment(lib, "Jde.QL.lib")
	#else
		#define ΓQL
	#endif
#endif