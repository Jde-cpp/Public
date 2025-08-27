#include <processthreadsapi.h>
#include <codecvt>
#include <jde/framework/thread/thread.h>
#include "../../Framework/source/threading/Thread.cpp"

constexpr Jde::ELogTags _tags{ Jde::ELogTags::Threads };

typedef HRESULT (WINAPI *TGetThreadDescription)( HANDLE, PWSTR* );
TGetThreadDescription pGetThreadDescription = nullptr;

typedef HRESULT (WINAPI *TSetThreadDescription)( HANDLE, PCWSTR );
TSetThreadDescription pSetThreadDescription = nullptr;

Ω initialize()ι->void{
	using namespace Jde;
	HMODULE hKernelBase = GetModuleHandleA("KernelBase.dll");
	if( !hKernelBase ){
		Critical{ _tags, "FATAL: failed to get kernel32.dll module handle, error:  {}", ::GetLastError() };
		return;
	}
	pSetThreadDescription = reinterpret_cast<TSetThreadDescription>( ::GetProcAddress(hKernelBase, "SetThreadDescription") );
	LOG_IF( !pSetThreadDescription, ELogLevel::Error, "FATAL: failed to get SetThreadDescription() address, error:  {:x}", ::GetLastError() );
	pGetThreadDescription = reinterpret_cast<TGetThreadDescription>( ::GetProcAddress(hKernelBase, "GetThreadDescription") );
	LOG_IF( !pGetThreadDescription, ELogLevel::Error, "FATAL: failed to get GetThreadDescription() address, error:  {:x}", ::GetLastError() );
}

Ω winSetThreadDscrptn( HANDLE h, Jde::sv ansiDescription )ι->void{
	using namespace Jde;
	let count = ::MultiByteToWideChar( CP_UTF8, 0, string{ansiDescription}.c_str() , -1, nullptr, 0 );
	auto wc = std::make_unique_for_overwrite<wchar_t[]>( count );
	::MultiByteToWideChar( CP_UTF8, 0, ansiDescription.data(), -1, wc.get(), count );
	if( !pSetThreadDescription )
		initialize();
	if( pSetThreadDescription && FAILED((*pSetThreadDescription)(h, wc.get())) )
		ERR( "({:x})Could not set name for thread: '{}', h: {:x}", ::GetLastError(), ansiDescription, (uint)h );
}

α Jde::SetThreadDscrptn( std::thread& thread, sv pszDescription )ι->void{
	winSetThreadDscrptn( static_cast<HANDLE>(thread.native_handle()), pszDescription );
}
α Jde::SetThreadDscrptn( sv description )ι->void{
	winSetThreadDscrptn( ::GetCurrentThread(), description );
}

α Jde::ThreadDscrptn()ι->const char*{
	if( std::strlen(ThreadName)>0 )
		return ThreadName;

	PWSTR pszThreadDescription;
	let threadId = ::GetCurrentThread();
	if( !pGetThreadDescription )
		initialize();
	if( !pGetThreadDescription )
		return ThreadName;
	let hr = (*pGetThreadDescription)( threadId, &pszThreadDescription );
	if( SUCCEEDED(hr) ){
		let size = wcslen(pszThreadDescription);
		auto pDescription = std::make_unique<char[]>( size+1 );
		uint size2;
		wcstombs_s( &size2, pDescription.get(), size+1, pszThreadDescription, size );
		::LocalFree( pszThreadDescription );
		std::strncpy( ThreadName, pDescription.get(), sizeof(ThreadName)/sizeof(ThreadName[0]) );
	}
	return ThreadName;
}

α Jde::ThreadId()ι->uint{
	if( !_threadId )
		_threadId = ::GetCurrentThreadId();
	return _threadId;
}
