#include <jde/fwk/process/thread.h>
#include <algorithm>
#include <atomic>
#include <sstream>

#define let const auto

namespace Jde{
	thread_local uint _threadId{0};

	constexpr uint NameLength = 256;
	thread_local char ThreadName[NameLength]={0};//string shows up as memory leak
	thread_local Threading::HThread AppThreadHandle{0};

	uint Threading::GetAppThreadHandle()ι{ return AppThreadHandle; }
	atomic<Threading::HThread> AppThreadHandleIndex{ (uint)Threading::EThread::AppSpecific };
	Threading::HThread Threading::BumpThreadHandle()ι{ return AppThreadHandleIndex++; }

	void Threading::SetThreadInfo( const ThreadParam& param )ι{
		AppThreadHandle = param.AppHandle;
	}
}