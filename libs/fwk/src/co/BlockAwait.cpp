#include <jde/fwk/co/Await.h>
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>

#define let const auto
namespace Jde{
	constexpr ELogTags _tags{ ELogTags::App | ELogTags::Threads };

	//How long a blocked caller waits before it starts saying so; 0 disables the warning.  The wait itself is never bounded
	//- see the note on BlockAwaitSync.
	Ω stallWarning()ι->Duration{
		static const Duration _interval = Settings::FindDuration( "/workers/blockStallWarning" ).value_or( Duration{30s} );
		return _interval;
	}

	α BlockAwaitSync::Signal()ι->void{
		{ lg _{_mutex}; _done = true; }
		_cv.notify_all();
	}

	α BlockAwaitSync::Wait( SL sl )ι->void{
		std::unique_lock l{ _mutex };
		let interval = stallWarning();
		if( interval<=Duration::zero() ){
			_cv.wait( l, [this](){return _done;} );
			return;
		}
		//Past the first interval this thread is parked on something that should already have answered.  It keeps waiting - the
		//caller's contract is a value, not a timeout - but says so every interval, and says where from: the whole cost of the
		//2026-08-07 gateway stall was that a dropped response looked exactly like a process quietly doing nothing.
		for( uint i=1; !_cv.wait_for(l, interval, [this](){return _done;}); ++i ){
			if( Process::Finalizing() )//the loggers are gone by then; a stall during finalize is the shutdown watchdog's problem.
				continue;
			//"{}" and a pre-formatted string, not "{:.1f}" and a double: Logging::Entry stringifies its arguments, so a spec
			//that only applies to a number makes Entry::Message() take its format-error path - which logs, from inside
			//MemoryLog::Find's write lock.
			LOGSL( i==1 ? ELogLevel::Warning : ELogLevel::Critical, sl, _tags, "BlockAwait has been waiting {}s for its result.", Ƒ("{:.1f}", std::chrono::duration<double>(interval*static_cast<Duration::rep>(i)).count()) );
		}
	}
}
