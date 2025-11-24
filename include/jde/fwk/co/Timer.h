#pragma once
#include <boost/asio.hpp>
#include "Await.h"

namespace Jde{
	using TimerAwait = TAwait<std::expected<void, boost::system::error_code>>;
	struct Γ DurationTimer final : TimerAwait{
		DurationTimer( steady_clock::duration duration, SRCE )ι;
		DurationTimer( steady_clock::duration duration, ELogTags tags, SRCE )ι;
		~DurationTimer();
		α await_ready()ι->bool override{ return _duration<steady_clock::duration::zero(); }
		α Suspend()ι->void override;
		α Cancel()ι->void{ _timer.cancel(); }
		α Restart()ε->void;
	private:
		sp<boost::asio::io_context> _ctx;
		steady_clock::duration _duration;
		mutex _mutex;
		ELogTags _tags;
		boost::asio::steady_timer _timer;
	};
}