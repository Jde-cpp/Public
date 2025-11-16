#pragma once
#include <jde/fwk/chrono.h>

namespace Jde{
	struct Stopwatch final {
		Stopwatch( string&& name, ELogTags tags, SRCE )ι: _name( std::move(name) ), _startTime( steady_clock::now() ), _tags( tags ), _sl{ sl }{}
		Stopwatch( ELogTags tags=ELogTags::App, SRCE )ι:Stopwatch{ {},tags,sl }{}

		~Stopwatch(){ LOGSL( ELogLevel::Trace, _sl, _tags, "[{}]{}", _name, Chrono::ToString(steady_clock::now() - _startTime) ); }

		α CheckTimeout( Duration timeout, Duration sleep={} )ε->void;
		α Reset()ι->void{ _startTime = steady_clock::now(); }
	private:
		string _name;
		steady_clock::time_point _startTime;
		ELogTags _tags;
		SL _sl;
	};

	α Stopwatch::CheckTimeout( Duration timeout, Duration sleep )ε->void{
		auto now = steady_clock::now();
		if( now - _startTime > timeout )
			THROW( "Timeout after {}", Chrono::ToString(timeout) );
		if( sleep!=Duration::zero() )
			std::this_thread::sleep_for( sleep );
	}
}
