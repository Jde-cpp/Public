#include <processthreadsapi.h>
#include <codecvt>
#include "../../Framework/source/threading/Thread.h"
#include "../../Framework/source/threading/Thread.cpp"

namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag("threads") };
	typedef HRESULT (WINAPI *TSetThreadDescription)( HANDLE, PCWSTR );
	TSetThreadDescription pSetThreadDescription = nullptr;

	typedef HRESULT (WINAPI *TGetThreadDescription)( HANDLE, PWSTR* );
	TGetThreadDescription pGetThreadDescription = nullptr;

	void Initialize()ι{
		HMODULE hKernelBase = GetModuleHandleA("KernelBase.dll"); RETURN_IF( !hKernelBase, ELogLevel::Critical, "FATAL: failed to get kernel32.dll module handle, error:  {}", ::GetLastError() );
		pSetThreadDescription = reinterpret_cast<TSetThreadDescription>( ::GetProcAddress(hKernelBase, "SetThreadDescription") );
		LOG_IF( !pSetThreadDescription, ELogLevel::Error, "FATAL: failed to get SetThreadDescription() address, error:  {:x}", ::GetLastError() );
		pGetThreadDescription = reinterpret_cast<TGetThreadDescription>( ::GetProcAddress(hKernelBase, "GetThreadDescription") );
		LOG_IF( !pGetThreadDescription, ELogLevel::Error, "FATAL: failed to get GetThreadDescription() address, error:  {:x}", ::GetLastError() );
	}
#pragma warning( disable: 4305 )
	void WinSetThreadDscrptn( HANDLE h, sv ansiDescription )ι{
		var count = ::MultiByteToWideChar( CP_UTF8, 0, string{ansiDescription}.c_str() , -1, NULL , 0 );
		vector<wchar_t> wc; wc.reserve( count );
		::MultiByteToWideChar( CP_UTF8, 0, ansiDescription.data(), -1, wc.data(), count );
		if( !pSetThreadDescription )
			Initialize();
		LOG_IF( pSetThreadDescription && FAILED((*pSetThreadDescription)(h, wc.data())), ELogLevel::Error, "Could not set name for thread({}) {} - (hr) - {} ", ansiDescription, h, ::GetLastError() );
	}

	uint Threading::GetThreadId()ι{
		return ThreadId ? ThreadId : ThreadId = ::GetCurrentThreadId();
	}

	void Threading::SetThreadDscrptn( std::thread& thread, sv pszDescription )ι{
		WinSetThreadDscrptn( static_cast<HANDLE>(thread.native_handle()), pszDescription );
	}
	void Threading::SetThreadDscrptn( sv description )ι{
		WinSetThreadDscrptn( ::GetCurrentThread(), description );
	}

	const char* Threading::GetThreadDescription()ι{
		if( std::strlen(ThreadName)==0 )
		{
			PWSTR pszThreadDescription;
			var threadId = ::GetCurrentThread();
			if( !pGetThreadDescription )
				Initialize();
			if( !pGetThreadDescription )
				return ThreadName;
			var hr = (*pGetThreadDescription)( threadId, &pszThreadDescription );
			if( SUCCEEDED(hr) )
			{
				var size = wcslen(pszThreadDescription);
				auto pDescription = std::make_unique<char[]>( size+1 );
				uint size2;
				wcstombs_s( &size2, pDescription.get(), size+1, pszThreadDescription, size );
				::LocalFree( pszThreadDescription );
				std::strncpy( ThreadName, pDescription.get(), sizeof(ThreadName)/sizeof(ThreadName[0]) );
			}
		}
		return ThreadName;
	}
}