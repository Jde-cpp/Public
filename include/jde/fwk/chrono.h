#pragma once
#ifndef CHRONO_H
#define CHRONO_H
#include <chrono>

#define Φ Γ auto
namespace Jde{
	using namespace std::chrono;
	template<class T=TimePoint>
	α ToIsoString( T timePoint )ι->string;
}
namespace Jde::Chrono{
	using namespace std::chrono;
	Ξ Epoch()ι->TimePoint{ return Clock::from_time_t(0); }
	Φ LocalTimeMilli( TimePoint time, SRCE )ε->string;
	Ξ LocalYMD( TimePoint time, const time_zone& tz )ι->year_month_day{ return year_month_day{floor<days>(tz.to_local(time))}; };
	Φ ToDuration( string&& iso, SRCE )ε->Duration;
	Ξ ToDuration( sv iso, SRCE )ε->Duration{ return ToDuration(string{iso}, sl); }
	Φ TryToDuration( string&& iso, ELogLevel level=ELogLevel::Debug, SRCE )ε->optional<Duration>;
	Φ ToTimePoint( string iso, SRCE )ε->TimePoint;
	Φ ToTimePoint( uint16_t year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, Duration subseconds=Duration::zero(), SRCE )ε->TimePoint;
	Φ ToTimeZone( sv name, const std::chrono::time_zone& dflt, ELogLevel level=ELogLevel::Debug, SRCE )ι->const std::chrono::time_zone&;
	template<class T=Duration>
	α ToString( T duration )ι->string;
	template<class To,class From>
	α ToClock( typename From::time_point from )ι->typename To::time_point{ return To::now()-milliseconds{duration_cast<milliseconds>(From::time_point::clock::now()-from)}; }
}

template<>
Ξ Jde::ToIsoString( steady_clock::time_point tp )ι->string{
	return Jde::ToIsoString<TimePoint>( Chrono::ToClock<Clock, steady_clock>(tp) );
}
Ŧ Jde::ToIsoString( T timePoint )ι->string{
	return std::chrono::duration_cast<std::chrono::milliseconds>( timePoint.time_since_epoch() ).count()%1000==0
		? std::format( "{:%FT%T}", std::chrono::floor<seconds>(timePoint) )
		: std::format( "{:%FT%T}", timePoint );
}
namespace Jde{
	Ŧ Chrono::ToString( T d )ι->string{
		//seconds-or-finer only: for coarser T (minutes, hours…) `d %= years{1}` in the output() macro is ill-formed,
		//since years/months aren't an exact multiple of T's period.  Every caller passes Duration/steady_clock ticks.
		static_assert( T::period::num==1, "Chrono::ToString requires a seconds-or-finer duration (T::period::num==1)." );
		std::ostringstream os;
		os << 'P';
		//date units use the exact std::chrono period ratios so ToDuration is the inverse - a month is ~730.5h, not 720h.
		#define output( period,suffix ) if( d>=period{1} || d<=period{-1} ){ os << duration_cast<period>(d).count() << suffix; d%=period{1}; }
		output( years, "Y" )
		output( months, "M" )
		output( days, "D" )
		if( d!=Duration::zero() ){
			os << "T";
			output( hours, "H" );
			output( minutes, "M" );
			if( d!=Duration::zero() ){
				auto ms = duration_cast<milliseconds>( d ).count(); //remainder <1min: emit as fractional seconds - ToDuration reads 'S' decimals, but drops a unitless tail.
				if( ms<0 ){ os << '-'; ms = -ms; }
				os << ms/1000;
				if( auto frac = ms%1000; frac ){
					os << '.';
					if( frac<100 ) os << '0';
					if( frac<10 ) os << '0';
					while( frac%10==0 ) frac/=10;
					os << frac;
				}
				os << 'S';
			}
		}
		return os.str();
	}
}
#undef output
#undef Φ
#endif
