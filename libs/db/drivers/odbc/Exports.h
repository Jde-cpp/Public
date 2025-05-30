#pragma once

#ifdef Jde_DB_Odbc_EXPORTS
	#ifdef _MSC_VER
		#define ΓODBC __declspec( dllexport )
	#else
		#define ΓODBC __attribute__((visibility("default")))
	#endif
#else 
	#ifdef _MSC_VER
		#define ΓODBC __declspec( dllimport )
		#if NDEBUG
			#pragma comment(lib, "Jde.DB.Odbc.lib")
		#else
			#pragma comment(lib, "Jde.DB.Odbc.lib")
		#endif
	#else
		#define ΓODBC
	#endif
#endif