#include "../../thread.cpp"
#include <sys/prctl.h>

α Jde::ThreadDscrptn()ι->const char*{
	if( std::strlen(ThreadName)==0 ){
		_threadId = pthread_self();
		if( let rc = pthread_getname_np( _threadId, ThreadName, NameLength ); rc != 0 )
			ERRT( ELogTags::Threads, "pthread_getname_np returned {}"sv, rc );
	}
	return ThreadName;
}

α Jde::SetThreadDscrptn( std::thread& thread, sv pszDescription )ι->void{
		pthread_setname_np( thread.native_handle(), string(pszDescription).c_str() );
}

α Jde::SetThreadDscrptn( sv description )ι->void{
	strncpy( ThreadName, string{description}.c_str(), NameLength-1 );
	prctl( PR_SET_NAME, ThreadName, 0, 0, 0 );
	_threadId = pthread_self();
}

α Jde::ThreadId()ι->uint{
	return _threadId ? _threadId : _threadId = pthread_self();
}