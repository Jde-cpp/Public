#pragma once
#include "./Assert.h"

#ifndef _MSC_VER
	namespace Jde
	{
		typedef void* HMODULE;
		typedef void* FARPROC;
	}
#endif

namespace Jde
{
	//https://blog.benoitblanchon.fr/getprocaddress-like-a-boss/
	struct ProcPtr
	{
		ProcPtr( FARPROC ptr ):
			_ptr(ptr)
		{}

		template <typename T, typename = std::enable_if_t<std::is_function_v<T>>>
		operator T *() const
		{
			return reinterpret_cast<T *>(_ptr);
		}
	private:
		FARPROC _ptr;
	};
	struct DllHelper
	{
		DllHelper( path path )noexcept(false):
			_path{path},
			_module{ OSApp::LoadLibrary(path) }
		{}
// #if _MSC_VER
// 			_module{ ::LoadLibrary(path.string().c_str()) }
// #else
// 			_module{ ::dlopen( path.c_str(), RTLD_LAZY ) }
// #endif
// 		{
// 			if( !_module )
// #ifdef _MSC_VER
// 				THROW( IOException("Can not load library '{}' - '{:x}'", path.string(), GetLastError()) );
// #else
// 				THROW( IOException("Can not load library '{}':  '{}'"sv, path.c_str(), dlerror()) );
// #endif
// 			INFO( "({})Opened"sv, path.string() );
//		}
		~DllHelper()
		{
			LOGX( ELogLevel::Information, "({})Freeing", _path.string() );
			OSApp::FreeLibrary( _module );
// #if _MSC_VER
// 			::FreeLibrary( _module );
// #else
// 			::dlclose( _module );
// #endif
			LOGX( ELogLevel::Information, "({})Freed", _path.string() );
		}


		ProcPtr operator[](str procName)const noexcept(false)
		{
// #if _MSC_VER
// 			auto procAddress = ::GetProcAddress( _module, string(proc_name).c_str() );
// #else
// 			auto procAddress = ::dlsym( _module, string(proc_name).c_str() );
// #endif
// 			CHECK( procAddress );
			return ProcPtr( OSApp::GetProcAddress(_module, procName) );
		}
	private:
		fs::path _path;
		HMODULE _module;
	};
}