#include <jde/fwk/chrono.h>
#include <jde/fwk/utils/mathUtils.h>

#define let const auto
namespace Jde{
	α Chrono::LocalTimeMilli( TimePoint time, SL sl )ε->string{
		try{
			auto tp = std::chrono::current_zone()->to_local( time );
			return std::format( "{0:%H:%M:}{1:%S}", tp, time.time_since_epoch() );
		}
		catch( const std::exception& e ){
			throw Exception{ sl, "Failed to convert time point to local time: {} - {}", ToIsoString(time), e.what() };
		}
	}
	α Chrono::ToDuration( string&& iso, SL sl )ε->Duration{
		std::istringstream is{ move(iso) };
		if( let ch = is.get(); ch!='P' ){
			string content{ (char)ch }; string line;
			while( std::getline(is,line) )
				content += line;
			throw Exception{ sl, ELogLevel::Debug, "Expected 'P' as first character in '{}{}'.", ch, content };
		}
		bool parsingTime = false;
		Duration duration{ Duration::zero() };
		while( is.good() ){
			if( !parsingTime && (parsingTime = is.peek()=='T') ){
				is.get();
				continue;
			}
			double value;
			is >> value;
			let type = is.get();
			if( type=='Y' )
				duration += hours( Round(value*365.25*24) );
			else if( !parsingTime && type=='M' )
				duration += hours( Round(value*30*24) );
			else if( type=='D' )
				duration += hours( Round(value*24) );
			else if( type=='H' )
				duration += minutes( Round(value*60) );
			else if( type=='M' )
				duration += seconds( Round(value*60) );
			else if( type=='S' )
				duration += milliseconds( Round(value*1000) );
		}
		return duration;
	}
	α Chrono::TryToDuration( string&& iso, ELogLevel level, SL sl )ε->optional<Duration>{
		try{
			return ToDuration( move(iso), sl );
		}
		catch( IException& e ){
			e.SetLevel( level );
		}
		return {};
	}
	α Chrono::ToTimePoint( string iso, SL sl )ε->TimePoint{
		TimePoint tp;
		std::istringstream is{ iso };
		is >> std::chrono::parse( "%FT%T", tp );
		THROW_IFSL( is.fail(), "Could not parse ISO time:  {}", iso );
		return tp;
	}
	α Chrono::ToTimePoint( uint16_t y, uint8_t mnth, uint8_t dayOfMonth, uint8 h, uint8 mnt, uint8 scnd, Duration subseconds, SL sl )ε->TimePoint{
    auto ymd = year{y}/month{mnth}/day{dayOfMonth};
		THROW_IFSL( !ymd.ok(), "Invalid date: {}-{}-{}", y, mnth, dayOfMonth );
    auto tp = sys_days{ymd} + hours{h} + minutes{mnt} + seconds{scnd}+subseconds;
		return tp;
	}
	α Chrono::ToTimeZone( sv name, const std::chrono::time_zone& dflt, ELogLevel level, SL sl )ι->const std::chrono::time_zone&{
		try{
			return *std::chrono::locate_zone( name );
		}
		catch( const std::runtime_error& e ){
			LOGSL( level, sl, ELogTags::Parsing, "Time zone: '{}' not found: {}. Using default '{}'.", name, e.what(), dflt.name() );
		}
		return dflt;
	}
}