#pragma once

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
		operator T*() const{ return reinterpret_cast<T *>(_ptr); }
	private:
		FARPROC _ptr;
	};
	struct DllHelper
	{
		DllHelper( path path )noexcept(false):
			_path{path},
			_module{ (HMODULE)OSApp::LoadLibrary(path) }
		{}

		~DllHelper()
		{
			LOGX( ELogLevel::Information, "({})Freeing", _path.string() );
			OSApp::FreeLibrary( _module );
			LOGX( ELogLevel::Information, "({})Freed", _path.string() );
		}

		α operator[](str procName)const noexcept(false)->ProcPtr
		{
			return ProcPtr( (FARPROC)OSApp::GetProcAddress(_module, procName) );
		}
	private:
		fs::path _path;
		HMODULE _module;
	};
}