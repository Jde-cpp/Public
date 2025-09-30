#pragma once
#ifdef Jde_Crypto_EXPORTS
	#ifdef _MSC_VER 
		#define ΓC __declspec( dllexport )
	#else
		#define ΓC __attribute__((visibility("default")))
	#endif
#else 
	#if defined(_MSC_VER) && defined(_WINDLL)
		#define ΓC __declspec( dllimport )
	#else
		#define ΓC
	#endif
#endif