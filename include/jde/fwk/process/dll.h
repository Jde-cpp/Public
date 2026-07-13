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
}