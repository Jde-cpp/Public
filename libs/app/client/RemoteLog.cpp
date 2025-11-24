#include <jde/app/client/RemoteLog.h>
#include <jde/fwk/process/execution.h>
#include <jde/app/client/IAppClient.h>

#define let const auto

namespace Jde::App::Client{
	RemoteLog::RemoteLog( const jobject& settings, sp<IAppClient> client )ι:
		ILogger{ settings },
		_client{ move(client) },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) }{
	}
	RemoteLog::~RemoteLog(){
		ASSERT( !_timer );
		if( _timer ){
			ResetTimer();
			while( _timer )
				std::this_thread::sleep_for(1ms);
		}
	}
	α RemoteLog::Start( sp<IAppClient> client )ι->void{
		_client = move(client);
		Executor();//locks up if starts in StartTimer.
		Execution::Run();
	}
	α RemoteLog::Shutdown( bool terminate )ι->void{
		_delay = Duration::min();
		ResetTimer();
		if( !terminate )
			Send();
		_client = nullptr;
	}
	α RemoteLog::Init( sp<IAppClient> client )ι->void{
		if( auto log = Logging::Add<RemoteLog>( "remote", move(client) ); log )
			log->Start( move(client) );
	}

	α RemoteLog::Write( const Logging::Entry& m )ι->void{
		if( !empty(m.Tags & _tags) )//recursion guard
			return;
		_mutex.lock();
		_entries.push_back( m );
		if( !_timer )
			StartTimer( _mutex );
	}
	α RemoteLog::StartTimer( std::mutex& mtx )ι->TimerAwait::Task{
		if( _delay<=Duration::zero() )
			co_return;
		_timer = mu<DurationTimer>( _delay, _tags, SRCE_CUR );
		mtx.unlock();
		let timedOut = co_await *_timer;
		{
			lg _{ mtx };
			_timer.reset();
		}
		if( timedOut )
			Send();
		else{
			lg _{ _mutex };
			if( _entries.size() )
				StartTimer( _mutex );
		}
	}

	α RemoteLog::ResetTimer()ι->void{
		lg _{_mutex};
		if( _timer )
			_timer->Cancel();
	}
	α RemoteLog::Send()ι->void{
		lg _{_mutex};
		ASSERT( _client );
		if( _client )
			_client->Write( move(_entries) );
	}
}