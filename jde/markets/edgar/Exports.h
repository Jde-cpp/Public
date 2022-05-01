#pragma once
#ifdef JdeEdgar_EXPORTS
	#ifdef _MSC_VER
		#define ΓE __declspec( dllexport )
	#else
		#define ΓE __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓE __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Markets.Edgar.lib")
		#else
			#pragma comment(lib, "Jde.Markets.Edgar.lib")
		#endif
	#else
		#define ΓE
	#endif
#endif