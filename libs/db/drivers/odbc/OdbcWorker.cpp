#include "OdbcWorker.h"
#include "../../../../libs/framework/src/process/os/windows/WindowsWorker.h"

namespace Jde::DB::Odbc{
	uint8 OdbcWorker::_threadCount{ std::numeric_limits<uint8>::max() };

	α OdbcWorker::Push( std::coroutine_handle<>&& h, HANDLE hEvent, bool close )ι->void{
		if( IWorker::ThreadCount<OdbcWorker>()==0 )
			Windows::WindowsWorkerMain::Push( move(h), hEvent, close );
	}
}