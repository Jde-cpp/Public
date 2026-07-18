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
			THROWSL( "Failed to convert time point to local time: {} - {}", ToIsoString(time), e.what() );
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
		while( true ){
			let c = is.peek();
			if( c==std::char_traits<char>::eof() )
				break;
			if( !parsingTime && c=='T' ){
				parsingTime = true;
				is.get();
				continue;
			}
			//scan the number digit-by-digit: istream>>double treats a trailing 'D' (a hex digit) as part of the
			//number candidate and then fails, silently swallowing the days token - so read only decimal chars.
			string num;
			if( c=='+' || c=='-' )
				num += (char)is.get();
			for( auto d=is.peek(); (d>='0'&&d<='9')||d=='.'; d=is.peek() )
				num += (char)is.get();
			let type = is.get();
			if( num.empty() || type==std::char_traits<char>::eof() )
				break;
			let value = std::stod( num );
			//date units use the exact std::chrono period ratios so ToDuration is the inverse of ToString
			//(which emits years/months/days) - a month is ~730.5h, not 720h; a year 365.2425d, not 365.25d.
			if( type=='Y' )
				duration += duration_cast<Duration>( std::chrono::duration<double,years::period>{value} );
			else if( !parsingTime && type=='M' )
				duration += duration_cast<Duration>( std::chrono::duration<double,months::period>{value} );
			else if( type=='D' )
				duration += duration_cast<Duration>( std::chrono::duration<double,days::period>{value} );
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
		catch( Exception& e ){
			e.SetLevel( level );
		}
		return {};
	}

	α Chrono::ToTimePoint( string iso, SL sl )ε->TimePoint{
		#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
			TimePoint tp;
			//an optional zone follows the seconds: 'Z' (UTC) or a numeric ±hh[:mm] offset. %FT%T reads the
			//date/time fields as UTC; a numeric offset is subtracted explicitly to normalize (local = UTC + offset).
			let zonePos = iso.size()>19 ? iso.find_first_of("+-", 19) : string::npos;
			if( !iso.empty() && iso.back()=='Z' ){
				std::istringstream is{ iso.substr(0, iso.size()-1) };
				is >> std::chrono::parse( "%FT%T", tp );
				THROW_IFSL( is.fail(), "Could not parse ISO time:  {}", iso );
			}
			else if( zonePos!=string::npos ){
				std::istringstream is{ iso.substr(0, zonePos) };
				is >> std::chrono::parse( "%FT%T", tp );
				THROW_IFSL( is.fail(), "Could not parse ISO time:  {}", iso );
				let off = iso.substr( zonePos );
				let sign = off[0]=='-' ? -1 : 1;
				string digits; for( char ch : off.substr(1) ) if( ch!=':' ) digits += ch;
				let oh = digits.size()>=2 ? std::stoi(digits.substr(0,2)) : 0;
				let om = digits.size()>=4 ? std::stoi(digits.substr(2,2)) : 0;
				tp -= sign*( hours{oh}+minutes{om} );
			}
			else{
				std::istringstream is{ iso };
				is >> std::chrono::parse( "%FT%T", tp );
				THROW_IFSL( is.fail(), "Could not parse ISO time:  {}", iso );
			}
			return tp;
		#else
			if( !iso.empty() && iso.back()=='Z' )//UTC designator; timegm below already treats the fields as UTC.
				iso.pop_back();
			std::istringstream is{ iso };
			std::tm tm{};
			is >> std::get_time( &tm, "%Y-%m-%dT%H:%M:%S" );
			THROW_IFSL( is.fail(), "Could not parse ISO time:  {}", iso );
			// timegm interprets tm as UTC, matching the %FT%T sys_time branch (mktime would apply the local tz offset).
			auto tp = std::chrono::system_clock::from_time_t( ::timegm(&tm) );
			if( iso.size()>19 && iso[19]=='.' )
				tp += std::chrono::milliseconds( Round(std::stod( iso.substr(19) )*1000) );
			if( let zonePos = iso.size()>19 ? iso.find_first_of("+-", 19) : string::npos; zonePos!=string::npos ){//numeric offset: local = UTC + offset, so subtract it to normalize.
				let off = iso.substr( zonePos );
				let sign = off[0]=='-' ? -1 : 1;
				string digits; for( char ch : off.substr(1) ) if( ch!=':' ) digits += ch;
				let oh = digits.size()>=2 ? std::stoi(digits.substr(0,2)) : 0;
				let om = digits.size()>=4 ? std::stoi(digits.substr(2,2)) : 0;
				tp -= sign*( hours{oh}+minutes{om} );
			}
			return tp;
		#endif
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