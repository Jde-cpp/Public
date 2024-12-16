#pragma once
#ifdef Jde_Opc_EXPORTS
	#ifdef _MSC_VER
		#define ΓOPC __declspec( dllexport )
		#define Jde_Opc_EXPORTS __declspec( dllexport )
	#else
		#define ΓOPC __attribute__( (visibility("default")) )
		#define Jde_Opc_EXPORTS __attribute__( (visibility("default")) )
	#endif
#else
	#ifdef _MSC_VER
		#define ΓOPC __declspec( dllimport )
		#define Jde_Opc_EXPORTS __declspec( dllimport )
		#if NDEBUG
			#pragma comment( lib, "Jde.Opc.lib" )
		#else
			#pragma comment( lib, "Jde.Opc.lib" )
		#endif
	#else
		#define ΓOPC
		#define Jde_Opc_EXPORTS
	#endif
#endif