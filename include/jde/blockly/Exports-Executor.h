#pragma once
//static_assert(false, "this function has to be implemented for desired type");

#ifdef JdeBlocklyExecutor_EXPORTS
	#ifdef _MSC_VER
		#define ΓBE __declspec( dllexport )
	#else
		#define ΓBE __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓBE __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Blockly.Executor.lib")
		#else
			#pragma comment(lib, "Jde.Blockly.Executor.lib")
		#endif
	#else
		#define ΓBE
	#endif
#endif 