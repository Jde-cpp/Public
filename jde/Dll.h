#pragma once
#include "./Assert.h"

#ifndef _MSC_VER
	#include <dlfcn.h>
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
		DllHelper( path path ):
			_path{path},
#if _MSC_VER
			_module{ ::LoadLibrary(path.string().c_str()) }
#else
			_module{ ::dlopen( path.c_str(), RTLD_LAZY ) }
#endif
		{
			if( !_module )
#ifdef _MSC_VER
				THROW( IOException("Can not load library '{}' - '{:x}'", path.string(), GetLastError()) );
#else
				THROW( IOException("Can not load library '{}':  '{}'"sv, path.c_str(), dlerror()) );
#endif
			DBG( "Opened module '{}'."sv, path.string() );
		}
		~DllHelper()
		{
			if( GetDefaultLogger() )
				DBGN( "Freeing '{}'."sv, _path.string() );
#if _MSC_VER
			::FreeLibrary( _module );
#else
			::dlclose( _module );
#endif
			if( GetDefaultLogger() )
				DBGN( "Freed '{}'."sv, _path.string() );
		}


		ProcPtr operator[](sv proc_name) const noexcept(false)
		{
#if _MSC_VER
			auto procAddress = ::GetProcAddress( _module, string(proc_name).c_str() );
#else
			auto procAddress = ::dlsym( _module, string(proc_name).c_str() );
#endif
			CHECK( procAddress );
			return ProcPtr( procAddress );
		}
	private:
		fs::path _path;
		HMODULE _module;
	};
}