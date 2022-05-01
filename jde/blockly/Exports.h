#pragma once

#ifdef JDE_BLOCKLY_EXPORTS
	#ifdef _MSC_VER
		#define ΓB __declspec( dllexport )
	#else
		#define ΓB __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓB __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Blockly.lib")
		#else
			#pragma comment(lib, "Jde.Blockly.lib")
		#endif
	#else
		#define ΓB
	#endif
#endif