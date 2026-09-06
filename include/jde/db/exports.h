#pragma once
#ifdef Jde_DB_EXPORTS
	#ifdef _MSC_VER
		#define ΓDB __declspec( dllexport )
	#else
		#define ΓDB __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓDB __declspec( dllimport )
	#else
		#define ΓDB
	#endif
#endif