#pragma once

#ifdef JDE_MYSQL_EXPORTS
	#ifdef _MSC_VER
		#define ΓMY __declspec( dllexport )
	#else
		#define ΓMY __attribute__((visibility("default")))
	#endif
#else
	#ifdef _MSC_VER
		#define ΓMY __declspec( dllimport )
		// #if NDEBUG
		// 	#pragma comment(lib, "Jde.DB.MySql.lib")
		// #else
		// 	#pragma comment(lib, "Jde.DB.MySql.lib")
		// #endif
	#else
		#define ΓMY
	#endif
#endif