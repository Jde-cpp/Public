#pragma once
#include <jde/framework/chrono.h>

namespace Jde{
	struct Stopwatch final {
		Stopwatch( string&& name, ELogTags tags, SRCE )ι: _name(std::move(name)), _startTime(steady_clock::now()), _tags(tags), _sl{sl}{}

		~Stopwatch() {
			Log( ELogLevel::Trace, _tags, _sl, "[{}]{}", _name, Chrono::ToString(steady_clock::now() - _startTime) );
		}
		string _name;
		steady_clock::time_point _startTime;
		ELogTags _tags;
		SL _sl;
	};
}
