#include <jde/fwk/str.h>
#include <algorithm>
#include <functional>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/string_generator.hpp>
#include <fmt/args.h>

#ifdef _MSC_VER
	#include <jde/fwk/process/os/windows/WindowsUtilities.h>
#endif

boost::uuids::string_generator _gen;
α Jde::ToUuid( sv s, SL sl )ε->uuid{
	try{
		return _gen( s.data(), s.data()+s.size() );
	}
	catch( const std::runtime_error& e ){
		throw Exception( ELogTags::Parsing, sl, "Invalid UUID string '{}': {}", s, e.what() );
	}
}

namespace Jde{
	const string _empty;
	α Str::Empty()ι->str{ return _empty; };

	α Str::DecodeUri( sv x )ι->string{
		auto from_hex = [](char ch) { return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10; };
		string y{}; y.reserve( x.size() );
    for (auto i = x.begin(), n = x.end(); i != n; ++i){
			char ch = *i;
      if( ch == '%' ){
        if (i[1] && i[2]){
          ch = (char)( from_hex(i[1]) << 4 | from_hex(i[2]) );
          i += 2;
        }
      }
			else if (ch == '+')
        ch = ' ';
			y+=ch;
		}
		return y;
	}
	α Str::Format( sv format, vector<string> args )ε->string{
    fmt::dynamic_format_arg_store<fmt::format_context> store;
		for( auto&& arg : args )
			store.push_back( move(arg) );
    return fmt::vformat(format, store);
	}
	α Str::TryFormat( sv format, vector<string> args )ι->string{
		try{
			return Str::Format( format, move(args) );
		}
		catch( const std::exception& e ){
			string msg = Ƒ("{}[{}]", format, Join(args, ", "));
			DBGT( ELogTags::Parsing, "Format error: {}, error: {}", msg, e.what() );
			return string{ msg };
		}
	}
	α Str::Replace( sv source, char find_, char replace )ι->string{
		string result{ source };
		for( char* pFrom=(char*)source.data(), *pTo=(char*)result.data(); pFrom<source.data()+source.size(); ++pFrom, ++pTo )
			*pTo = *pFrom==find_ ? replace : *pFrom;

		return result;
	}
	α Str::Replace( sv source, sv find, sv replace )ι->string{
		string y; y.reserve( source.size() ); uint iLast{ 0 };
		for( uint i{}; (i = source.find(find, i))!=string::npos; iLast = (i=i+find.length()) ){
			y += source.substr( iLast, i-iLast );
			y += replace;
		}
		if( iLast<source.size() )
			y += source.substr( iLast, source.size()-iLast );

		return y;
	}
	α Str::Split( sv s, char delim )ι->vector<sv>{
		vector<sv> y;
		for( uint fieldStart=0, fieldEnd;fieldStart<s.size();fieldStart = fieldEnd+1 ){
			fieldEnd = std::min( s.find_first_of(delim, fieldStart), s.size() );
			sv v{s.data()+fieldStart, fieldEnd-fieldStart };
			if( v.size() )
				y.push_back( v );
		}
		return y;
	}
	α Str::ToHex( byte* p, uint size )ι->string{
		string hex;
		hex.reserve( size*2 );
		boost::algorithm::hex_lower( (char*)p, (char*)p+size, std::back_inserter(hex) );
		return hex;
	}

#pragma warning( disable: 4244 )
	Ω transform( sv source, function<int(int)> fnctn )ι->string{
		string result{ source };
		std::transform( result.begin(), result.end(), result.begin(), fnctn );
		return result;
	}
	α Str::ToLower( sv source )ι->string{ return transform( source, ::tolower ); }
	α Str::ToUpper( sv source )ι->string{ return transform( source, ::toupper ); }


	α GetChar( sv x, uint& i )ι->wchar_t{
		wchar_t ch = i<x.size() ? x[i] : '\0';
		if( ch==L'\xffe2' && i+2<x.size() ){
			ch =  x[++i];
			ch = ch << 8;
			wchar_t lo = x[++i] & 0xff; //lo &= 0xff;
			ch += lo;
		}
		return ch;
	};

	α Str::LTrim( sv s )->sv{
		uint i=0; wchar_t ch;
		for( ch = GetChar( s, i );
			std::isspace(ch) || ch==L'\x80af' || ch==L'\x8093';
			ch = GetChar(s, ++i) );
		if( ch>0xff && i>1 )
			i-=2;
		return i ? sv{ s.data()+i, s.size()-i } : s;
	}


	Ω rTrim( sv s, function<bool(char)> f )->sv{
		sv y;
		if( auto p = std::find_if(s.rbegin(), s.rend(), f); p!=s.rend() )
			y = p==s.rbegin() ? s : sv{ s.data(), s.size()-std::distance(s.rbegin(), p) };
		return y;
	}
	α Str::RTrim( sv s )->sv{
		return rTrim( s, [](int ch){return !std::isspace(ch);} );
	}

	α Str::StartsWithInsensitive( sv value, sv starting )ι->bool{
		bool equal = starting.size() <= value.size();
		if( equal ){
			for( sv::size_type i=0; i<starting.size(); ++i ){
				equal = ::toupper(starting[i])==::toupper(value[i]);
				if( !equal )
					break;
			}
		}
		return equal;
	}
}