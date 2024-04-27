#pragma once
#ifdef Jde_Iot_EXPORTS
	#ifdef _MSC_VER
		#define ΓI __declspec( dllexport )
	#else
		#define ΓI __attribute__( (visibility("default")) )
	#endif
#else
	#ifdef _MSC_VER
		#define ΓI __declspec( dllimport )
		#if NDEBUG
			#pragma comment( lib, "Jde.Iot.lib" )
		#else
			#pragma comment( lib, "Jde.Iot.lib" )
		#endif
	#else
		#define ΓI
	#endif
#endif