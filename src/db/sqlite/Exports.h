#pragma once

#ifdef JDE_SQLITE_EXPORTS
	#ifdef _MSC_VER
		#define ΓITE __declspec( dllexport )
	#else
		#define ΓITE __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define ΓITE __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.DB.Sqlite.lib")
		#else
			#pragma comment(lib, "Jde.DB.Sqlite.lib")
		#endif
	#else
		#define ΓITE
	#endif
#endif