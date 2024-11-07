#pragma once
#ifdef Jde_Crypto_EXPORTS
	#ifdef _MSC_VER 
		#define ΓC __declspec( dllexport )
	#else
		#define ΓC __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define ΓC __declspec( dllimport )
		#pragma comment(lib, "Jde.Crypto.lib")
	#else
		#define ΓC
	#endif
#endif