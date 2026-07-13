#pragma once
#include "process.h"

#ifndef _MSC_VER
	namespace Jde{
		typedef void* HMODULE;
		typedef void* FARPROC;
	}
#endif

namespace Jde{
	//https://blog.benoitblanchon.fr/getprocaddress-like-a-boss/
	struct ProcPtr{
		ProcPtr( FARPROC ptr ):
			_ptr(ptr)
		{}

		template <typename T, typename = std::enable_if_t<std::is_function_v<T>>>
		operator T*() const{ return reinterpret_cast<T *>(_ptr); }
	private:
		FARPROC _ptr;
	};
#undef LoadLibrary
	struct DllHelper{
		DllHelper( fs::path&& path )ε:
			_path{move(path)},
			_module{ (HMODULE)Process::LoadLibrary(_path) }
		{}

		~DllHelper(){
			Process::FreeLibrary( _module );
		}

		α operator[](str procName)Ε->ProcPtr{
			return ProcPtr( (FARPROC)Process::GetProcAddress(_module, procName) );
		}
	private:
		fs::path _path;
		HMODULE _module;
	};

	//Process-wide cache of dll-backed api objects keyed by path - the same dll requested twice shares one T.
	//Weak entries: T is destroyed (and its dll freed) when the last owner releases it; hold the sp to keep the api loaded.
	template<class T>
	struct DllApiCache{
		α Get( const fs::path& path )ε->sp<T>{
			lg _{ _mutex };
			auto& cached = _apis[path];
			auto api = cached.lock();
			if( !api )
				cached = api = ms<T>( path );
			return api;
		}
	private:
		flat_map<fs::path,wp<T>> _apis;
		std::mutex _mutex;
	};
}