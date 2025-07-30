#pragma once
#include <boost/asio.hpp>
#include "Await.h"

namespace Jde{
	struct Γ DurationTimer final : VoidAwait{
		DurationTimer( steady_clock::duration duration, SRCE )ι;
		α await_ready()ι->bool override{ return _duration<steady_clock::duration::zero(); }
		α Suspend()ι->void override;
		α Restart()ι->void;
	private:
		steady_clock::duration _duration;
		sp<boost::asio::io_context> _ctx;
		boost::asio::steady_timer _timer;
	};
}