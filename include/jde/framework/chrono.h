#pragma once
#ifndef CHRONO_H
#define CHRONO_H
#include <chrono>

#define Φ Γ auto
namespace Jde{
	template<class T=TimePoint>
	α ToIsoString( T timePoint )ι->string;
}
namespace Jde::Chrono{
	using namespace std::chrono;
	α LocalTimeMilli( TimePoint time, SRCE )ε->string;
	Φ ToDuration( string&& iso )ε->Duration;
	Ξ ToDuration( sv iso )ε->Duration{ return ToDuration(string{iso}); }
	α ToTimePoint( string iso, SRCE )ε->TimePoint;
	α ToTimePoint( uint16_t year, uint8 month, uint8 day, uint8 hour=0, uint8 minute=0, uint8 second=0, SRCE )ε->TimePoint;
	template<class T=Duration>
	α ToString( T duration )ι->string;
	template<class To,class From>
	α ToClock( typename From::time_point from )ι->typename To::time_point{ return To::now()-milliseconds{ duration_cast<milliseconds>(From::time_point::clock::now()-from) }; }
}

template<>
Ξ Jde::ToIsoString( steady_clock::time_point tp )ι->string{
	return Jde::ToIsoString<TimePoint>( Chrono::ToClock<Clock, steady_clock>(tp) );
}
Ŧ Jde::ToIsoString( T timePoint )ι->string{
	return std::format( "{:%Y-%m-%dT%H:%M:%S}", timePoint );
}
namespace Jde{
	Ŧ Chrono::ToString( T d )ι->string{
		std::ostringstream os;
		os << 'P';
		#define output(period,suffix) if( d>=period{1} || d<=period{-1} ){ os << duration_cast<period>(d).count() << suffix; d%=period{1}; }
		if constexpr( _msvc ){
			constexpr auto year = hours(24 * 365);
			if( d >= year || d <= -year ){
				os << duration_cast<hours>(d).count()/year.count() << "Y";
				d %= year;
			}
			constexpr auto month = hours(24 * 30);
			if( d >= month || d <= -month ){
				os << duration_cast<hours>(d).count() / month.count() << "M";
				d %= month;
			}
			constexpr auto days = hours(24);
			if( d >= days || d <= -days ){
				os << duration_cast<hours>(d).count() / days.count() << "M";
				d %= days;
			}
		}
		else{
			output( years, "Y" )
			output( months, "M" )
			output( days, "D" )
		}
		if( d!=Duration::zero() ){
			os << "T";
			output( hours, "H" );
			output( minutes, "M" );
			output( seconds, "S" );
			if( d!=Duration::zero() )
				os << duration_cast<milliseconds>(d).count();
		}
		return os.str();
	}
}
#undef Φ
#endif
