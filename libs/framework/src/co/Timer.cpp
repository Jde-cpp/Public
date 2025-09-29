#include <jde/framework/co/Timer.h>
#include <jde/framework/process/execution.h>


namespace Jde{
	DurationTimer::DurationTimer( steady_clock::duration duration, SL sl )ι:
		DurationTimer{ duration, ELogTags::Scheduler, sl }
	{}

	DurationTimer::DurationTimer( steady_clock::duration duration, ELogTags tags, SL sl )ι:
		VoidAwait{sl},
		_ctx{ Executor() },
		_duration{duration},
		_tags{tags},
		_timer{ *_ctx }
	{}

	DurationTimer::~DurationTimer(){
	}

	α DurationTimer::Restart()ε->void{
		lg _{_mutex};
		THROW_IF( !_h, "Already triggered." );
		_timer.expires_after( _duration );
	  _timer.async_wait([this](const boost::system::error_code& ec){
			if( !ec ){
				Resume();
			}
			else{
				ResumeExp( BoostCodeException{ec} );
			}
	  });
	}
	α DurationTimer::Suspend()ι->void{
		Restart();
		Execution::Run();
	}
}