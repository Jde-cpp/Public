#include <jde/fwk/str.h>
#include <algorithm>
#include <functional>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/args.h>

#ifdef _MSC_VER
	#include <jde/fwk/process/os/windows/WindowsUtilities.h>
#endif
#define let const auto
boost::uuids::string_generator _gen;
α Jde::ToUuid( sv s, SL sl )ε->uuid{
	try{
		return _gen( s.data(), s.data()+s.size() );
	}
	catch( const std::runtime_error& e ){
		throw Exception( sl, {ELogTags::Parsing}, "Invalid UUID string '{}': {}", s, e.what() );
	}
}
α Jde::ToString( const boost::uuids::uuid& u )ι->string{
	return boost::uuids::to_string( u );
}

namespace Jde{
	const string _empty;
	α Str::Empty()ι->str{ return _empty; };

	α Str::DecodeUri( sv x )ι->string{
		auto fromHex = []( char ch )->int {
			if( ch>='0' && ch<='9' )
				return ch-'0';
			ch = (char)::tolower( (unsigned char)ch );
			return ch>='a' && ch<='f' ? ch-'a'+10 : -1;
		};
		string y; y.reserve( x.size() );
		for( uint i=0; i<x.size(); ++i ){
			char ch = x[i];
			if( ch=='%' && i+2<x.size() ){//sv isn't null terminated - a trailing '%' must not read past the end.
				if( let hi = fromHex(x[i+1]), lo = fromHex(x[i+2]); hi>=0 && lo>=0 ){//invalid escapes pass through literally.
					ch = (char)( hi<<4 | lo );
					i += 2;
				}
			}
			else if( ch=='+' )
				ch = ' ';
			y += ch;
		}
		return y;
	}
	α Str::Format( sv format, vector<string> args )ε->string{
    fmt::dynamic_format_arg_store<fmt::format_context> store;
		for( auto&& arg : args )
			store.push_back( move(arg) );
    return fmt::vformat( format, store );
	}
	α Str::TryFormat( sv format, vector<string> args )ι->string{
		try{
			return Str::Format( format, move(args) );
		}
		catch( const std::exception& e ){
			string msg = Ƒ( "{}[{}]", format, Join(args, ", ") );
			DBGT( ELogTags::Parsing, "Format error: {}, error: {}", msg, e.what() );
			return string{ msg };
		}
	}
	α Str::Replace( sv source, char find_, char replace )ι->string{
		string result{ source };
		std::ranges::replace( result, find_, replace );
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
			sv v{ s.data()+fieldStart, fieldEnd-fieldStart };
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

	Ω transform( sv source, int(*f)(int) )ι->string{
		string result{ source };
		std::ranges::transform( result, result.begin(), [f](char ch){ return (char)f((unsigned char)ch); } );//unsigned cast: tolower/toupper are ub for negative chars.
		return result;
	}
	α Str::ToLower( sv source )ι->string{ return transform(source, ::tolower); }
	α Str::ToUpper( sv source )ι->string{ return transform(source, ::toupper); }


	//decode one code point at x[i]. Bytes read unsigned - a signed char sign-extends into a negative
	//wchar_t, which is UB in the space check. Only ASCII and the UTF-8 3-byte block led by 0xE2
	//(U+2000..U+2FFF, general punctuation - where Unicode spaces live) are decoded; else the raw byte 0..255.
	α GetChar( sv x, uint& i )ι->char32_t{
		if( i>=x.size() )
			return 0;
		let b = (unsigned char)x[i];
		if( b==0xE2 && i+2<x.size() ){
			let hi = (unsigned char)x[i+1] & 0x3Fu, lo = (unsigned char)x[i+2] & 0x3Fu;
			i += 2;
			return 0x2000u | (hi << 6) | lo;
		}
		return b;
	};

	Ω isSpace( char32_t ch )->bool{
		if( ch<0x80 )
			return std::isspace( (int)ch )!=0;
		return (ch>=0x2000 && ch<=0x200A) || ch==0x2028 || ch==0x2029 || ch==0x202F || ch==0x205F;//Unicode spaces in the general-punctuation block.
	}

	Ṫ ltrim( T&& s, function<bool(char32_t)> f )->T{
		uint i=0;
		for( ; i<s.size() && f((unsigned char)*(std::begin(s)+i)); ++i );//unsigned: byte-wise paths must not sign-extend into f.
		return i==0 ? s : T{ std::begin(s)+i, std::end(s) };
	}
	Ṫ rtrim( T&& s, function<bool(char32_t)> f )->T{
		uint i=s.size();
		for( ; i>0 && f((unsigned char)*(std::begin(s)+i-1)); --i );
		return i==s.size() ? s : T{ std::begin(s), std::begin(s)+i };
	}

	α Str::LTrim( string&& s )->string{
		return ltrim( move(s), isSpace );
	}
	α Str::LTrim( sv s )->sv{
		uint i=0; char32_t ch;
		for( ch = GetChar(s, i); isSpace(ch); ch = GetChar(s, ++i) );
		if( ch>0xff && i>1 )
			i-=2;
		return i ? sv{ s.data()+i, s.size()-i } : s;
	}
	α Str::TrimFirstLast( string&& s, char first, char last )ι->string{
		bool found{};
		auto f = [&found]( char bracket, char32_t ch ){
			let skip = ch==(char32_t)(unsigned char)bracket && !found;
			if( skip )
				found = true;
			return skip || isSpace( ch );
		};
		auto trim = ltrim( move(s), [&f, first](char32_t ch){return f(first, ch);} );
		if( !found )
			return RTrim( move(trim) );
		found = false;
		return rtrim( move(trim), [&f, last](char32_t ch){return f(last, ch);} );
	}

	α Str::RTrim( string&& s )->string{
		auto y = move( s );
		let trimmed = RTrim( y );
		return trimmed.size()==y.size() ? y : string{ trimmed };
	}
	α Str::RTrim( sv s )->sv{
		return rtrim( sv{s}, [](char32_t ch){return isSpace(ch);} );
	}

	α Str::StartsWithInsensitive( sv value, sv starting )ι->bool{
		bool equal = starting.size() <= value.size();
		if( equal ){
			for( sv::size_type i=0; i<starting.size(); ++i ){
				equal = ::toupper( starting[i] )==::toupper( value[i] );
				if( !equal )
					break;
			}
		}
		return equal;
	}
}