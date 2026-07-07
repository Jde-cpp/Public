//#include "../../thread.cpp"
#include "jde/fwk/process/thread.h"
//#include "jde/fwk/str.h"
#include "jde/fwk/usings.h"
#include <sys/prctl.h>

#define let const auto
constexpr Jde::ELogTags _tags{ Jde::ELogTags::Threads };
constexpr uint _nameLength = 15;

namespace Jde{
	α Thread::Id()ι->ProcessThreadId{
		return pthread_self();
	}

	α Thread::Name()ι->string{
		char threadName[_nameLength+1]{};
		if( let rc = pthread_getname_np(Id(), threadName, sizeof(threadName)); rc != 0 )//needs room for 15 chars + nul; passing 15 fails with ERANGE on 15-char names.
			ERR( "pthread_getname_np returned {}", rc );
		return threadName;
	}
	α Thread::SetName( Thread::ProcessThreadId id, sv description )ι->void{
		pthread_setname_np( id, string(description).substr(0, _nameLength).c_str() );
	}
	α Thread::SetName( std::thread& thread, sv pszDescription )ι->void{
		SetName( thread.native_handle(), pszDescription );
	}

	α Thread::SetName( sv name )ι->void{
		prctl( PR_SET_NAME, string{name}.substr(0, _nameLength).c_str(), 0, 0, 0 );
	}
}
