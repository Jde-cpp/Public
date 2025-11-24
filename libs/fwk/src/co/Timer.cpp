#include <jde/fwk/co/Timer.h>
#include <jde/fwk/process/execution.h>


namespace Jde{
	DurationTimer::DurationTimer( steady_clock::duration duration, SL sl )ι:
		DurationTimer{ duration, ELogTags::Scheduler, sl }
	{}

	DurationTimer::DurationTimer( steady_clock::duration duration, ELogTags tags, SL sl )ι:
		TimerAwait{sl},
		_ctx{ Executor() },
		_duration{duration},
		_tags{tags},
		_timer{ *_ctx }
	{}

	DurationTimer::~DurationTimer(){
		ASSERT( !_h && "Timer destroyed before awaited completion." );
	}

	α DurationTimer::Restart()ε->void{
		lg _{_mutex};
		THROW_IF( !_h, "Already triggered." );
		_timer.expires_after( _duration );
	  _timer.async_wait([h=_h](const boost::system::error_code& ec){
			if( !ec )
				h.promise().Resume( {}, h );
			else
				h.promise().Resume( std::unexpected{ec}, h );
	  });
	}
	α DurationTimer::Suspend()ι->void{
		Restart();
		Execution::Run();
	}
}