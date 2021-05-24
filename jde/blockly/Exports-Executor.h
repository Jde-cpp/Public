#pragma once
//static_assert(false, "this function has to be implemented for desired type");

#ifdef JDE_BLOCKLY_EXECUTOR_EXPORTS
	#ifdef _MSC_VER
		#define JDE_BLOCKLY_EXECUTOR __declspec( dllexport )
	#else
		#define JDE_BLOCKLY_EXECUTOR __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define JDE_BLOCKLY_EXECUTOR __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.Blockly.Executor.lib")
		#else
			#pragma comment(lib, "Jde.Blockly.Executor.lib")
		#endif
	#else
		#define JDE_BLOCKLY_EXECUTOR
	#endif
#endif 