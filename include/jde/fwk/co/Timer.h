#pragma once
#include <boost/asio.hpp>
#include <chrono>
#include "Await.h"
#include "jde/fwk/usings.h"

namespace Jde{
	using TimerAwait = TAwait<std::expected<void, boost::system::error_code>>;
	struct Γ DurationTimer final : TimerAwait{
		DurationTimer( steady_clock::duration duration, SRCE )ι;
		DurationTimer( steady_clock::duration duration, boost::asio::any_io_executor executor, SRCE )ι;//resumes the awaiter on `executor` (e.g. a strand) instead of an arbitrary pool thread.
		~DurationTimer();
		α await_ready()ι->bool override{ return _duration<steady_clock::duration::zero(); }
		α await_resume()ε->std::expected<void, boost::system::error_code> override{//ready path attaches no promise - complete successfully instead of the base's 'promise is null' throw.
			return _h ? TimerAwait::await_resume() : std::expected<void, boost::system::error_code>{};
		}
		α Suspend()ι->void override;
		α Cancel()ι->uint{ lg _{_mutex}; return _timer.cancel(); }//asio timers aren't safe for concurrent cancel vs Start's async_wait.
	private:
		α Start()ε->void;
		sp<boost::asio::io_context> _ctx;
		steady_clock::duration _duration;
		optional<boost::asio::any_io_executor> _executor;//when set, completion handlers are bound to it.
		mutex _mutex;
		boost::asio::steady_timer _timer;
	};
}