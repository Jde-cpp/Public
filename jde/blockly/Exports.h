#pragma once

#ifdef JDE_BLOCKLY_EXPORTS
	#ifdef _MSC_VER
		#define JDE_BLOCKLY __declspec( dllexport )
	#else
		#define JDE_BLOCKLY __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define JDE_BLOCKLY __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Blockly.lib")
		#else
			#pragma comment(lib, "Jde.Blockly.lib")
		#endif
	#else
		#define JDE_BLOCKLY
	#endif
#endif