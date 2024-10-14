#pragma once
#include <jde/Exception.h>
#include <jde/App.h>

namespace Jde{
  struct WinHandle final{
    WinHandle( std::nullptr_t=nullptr )ι : _value(nullptr) {}
		//TODO: forward exception?
    WinHandle( HANDLE value, function<IOException()> e )ε:
      _value( value ){
      if( _value==INVALID_HANDLE_VALUE )
          throw e();
    }

    explicit operator bool()Ι{ return _value != nullptr && _value != INVALID_HANDLE_VALUE; }
    operator HANDLE()Ι{ return _value; }
		struct Deleter final {
			using pointer=WinHandle;
			void operator()( WinHandle handle )Ι{ 
				if( handle && !::CloseHandle(handle) )
					Warning( ELogTags::App, "CloseHandle returned {}", ::GetLastError() ); 
			}
		};
	private:
    HANDLE _value;
  };
  using HandlePtr=std::unique_ptr<WinHandle, WinHandle::Deleter>;
}