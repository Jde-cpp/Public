#include <jde/app/client/RemoteLog.h>
#include <jde/fwk/process/execution.h>

namespace Jde::App::Client{
	#define let const auto
	RemoteLog::RemoteLog( const jobject& settings )ι:
		ILogger{ settings },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) }{
		Executor();//locks up if starts in StartTimer.
		Execution::Run();
		Process::AddShutdownFunction( [this]( bool /*terminate*/ ){	//member Shutdown gets called after timer thread shutdown.
			_delay = Duration::min();
			ResetTimer();
		});
	}

	α RemoteLog::Write( const Logging::Entry& m )ι->void{
		if( !empty(m.Tags & _tags) )//recursion guard
			return;
		_mutex.lock();
		_entries.push_back( m );
		if( !_timer )
			StartTimer();
	}
	α RemoteLog::StartTimer()ι->VoidAwait::Task{
		if( _delay<=Duration::zero() )
			co_return;
		_timer = mu<DurationTimer>( _delay, _tags, SRCE_CUR );
		try{
			co_await *_timer;
			_mutex.lock();
			Send();
		}
		catch( const IException& ){
			lg _{_mutex};
			if( _entries.size() )
				StartTimer();
			else
				_timer = nullptr;
		}
	}

	α RemoteLog::ResetTimer()ι->void{
		if( _timer )
			_timer->Cancel();
	}
	α RemoteLog::Send()->void{
		//add to IAppClient
	}
}