#pragma once

#ifdef JDE_SQLITE_EXPORTS
	#ifdef _MSC_VER
		#define ΓLITE __declspec( dllexport )
	#else
		#define ΓLITE __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓLITE __declspec( dllimport )
	#else
		#define ΓLITE
	#endif
#endif
