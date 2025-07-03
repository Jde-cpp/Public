#pragma once
#include "Await.h"

namespace Jde{
	struct Γ DurationTimer final : VoidAwait<>{
		DurationTimer( steady_clock::duration duration, SRCE )ι:VoidAwait<>{sl}, _duration{duration}{}
		α await_ready()ι->bool override{ return _duration<steady_clock::duration::zero(); }
		α Suspend()ι->void override;
	private:
		steady_clock::duration _duration;
	};
}