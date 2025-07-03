﻿#pragma once
#ifndef JDE_STR_H
#define JDE_STR_H
DISABLE_WARNINGS
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#include <charconv>
#include <codecvt>
#include <span>
#include <boost/algorithm/string/trim.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>
ENABLE_WARNINGS

#define Φ Γ auto
#define let const auto
namespace Jde{
	Ŧ To( sv value )ι->T{ T v{}; std::from_chars( value.data(), value.data()+value.size(), v ); return v; }
	template<> Ξ To( sv x )ι->double{ return stod( string{x} ); }
}
namespace Jde::Str{
	Φ Empty()ι->str;
	template<class T=string> α Decode64( sv s, bool convertFromFileSafe=false )ε->T;
	Φ DecodeUri( sv str )ι->string;
	template<class T, class I=T::const_iterator> α Encode64( const T& val, bool convertFromFileSafe=false )ι->string;

	Φ Format( sv format, vector<string> args )ι->string;
	Ŧ Join( T collection, sv separator=",", bool quote=false )ι->string;
	Φ Replace( sv source, sv find, sv replace )ι->string;
	Φ Replace( sv source, char find, char replace )ι->string;
	Φ Split( sv s, char delim=',' )ι->vector<sv>;
	Ξ StartsWith( sv value, sv starting )ι{ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
	α StartsWithInsensitive( sv value, sv starting )ι->bool;
	α LTrim( sv s )->sv;
	α RTrim( sv s )->sv;
	Φ ToHex( byte* p, uint size )ι->string;
	Φ ToLower( sv source )ι->string;
	Φ ToUpper( sv source )ι->string;
	template<class T=uint> α TryTo( str s, uint* pos = nullptr, int base = 10 )ι->optional<T>;

	Ξ Trim( sv s )->sv{ return RTrim( LTrim(s) ); }

	template<class Y=sv, class X> α ToView( const X& x )ι->Y{ return Y{x.data(),x.size()}; }
	template<class T> using bsv = std::basic_string_view<char,T>;
	template<class T=sv, class D=sv> α Split( bsv<typename T::traits_type> s, bsv<typename D::traits_type> delim )ι->vector<bsv<typename T::traits_type>>;

	struct ci_traits : public std::char_traits<char>{
		Ω eq(char c1, char c2)ι{ return toupper(c1) == toupper(c2); }
		Ω ne(char c1, char c2)ι{ return toupper(c1) != toupper(c2); }
		Ω lt(char c1, char c2)ι{ return toupper(c1) <  toupper(c2); }
		Ω compare( const char* s1, const char* s2, uint n )ι->int{
			int y{};
			for( ; !y && n-- != 0; ++s1, ++s2 ){
				if( toupper(*s1) < toupper(*s2) ) y=-1;
				if( toupper(*s1) > toupper(*s2) ) y= 1;
			}
			return y;
		}
		Ω find( const char* s, uint n, char a )ι->const char*{
			while( n-- > 0 && toupper(*s) != toupper(a) )
				++s;
			return n==string::npos ? nullptr : s;
		}
	};
	using iv = bsv<Str::ci_traits>;
}
namespace Jde{
	constexpr Str::iv operator "" _iv( const char* x, uint len )ι{ return Str::iv(x, len); }

	Ξ ToSV( Str::iv x )ι->sv{ return Str::ToView<sv,Str::iv>( x ); }
	Ξ ToIV( sv x )ι->Str::iv{ return Str::ToView<Str::iv,sv>( x ); }

	Ŧ Str::Decode64( sv s, bool convertFromFileSafe )ε->T{ //https://stackoverflow.com/questions/10521581/base64-encode-using-boost-throw-exception
		string encoded{ s };
		if( convertFromFileSafe )
			encoded = Str::Replace( Str::Replace(encoded, '_', '/'), '-', '+' );

		using namespace boost::archive::iterators;
		using TW = transform_width<binary_from_base64<remove_whitespace<string::const_iterator>>, 8, 6>;
		T y{ TW(encoded.begin()), TW(encoded.end()) };
		while( y.size() && y.back()==0 )
			y.pop_back();
		return y;
	}

	template<class T, class I> α Str::Encode64( const T& val, bool convertFromFileSafe )ι->string{//https://stackoverflow.com/questions/7053538/how-do-i-encode-a-string-to-base64-using-only-boost
		using namespace boost::archive::iterators;
		using It = base64_from_binary<transform_width<I, 6, 8>>;
		auto t = string{ It(std::begin(val)), It(std::end(val)) };
		auto encoded = t.append( (3 - val.size() % 3) % 3, '=' );
		if( convertFromFileSafe )
			encoded = Replace( Replace( encoded, '/', '_' ), '+', '-' );
		return encoded;
	}

	Ŧ Str::Join( T collection, sv separator, bool quote )ι->string{
		std::ostringstream os;
		auto first = true;
		for( const auto& item : collection ){
			if( first )
				first = false;
			else
				os << separator;
			if( quote )
				os << '"' << item << '"';
			else
				os << item;
		}
		return os.str();
	}

	template<class T, class D> α Str::Split( bsv<typename T::traits_type> s_, bsv<typename D::traits_type> delim )ι->vector<bsv<typename T::traits_type>>{
		vector<bsv<typename T::traits_type>> tokens;
		if( s_.empty() )
			return tokens;
		uint i=0;
		bsv<typename D::traits_type> s{ s_.data(), s_.size() };
		for( uint next = s.find(delim); next!=string::npos; i=next+delim.size(), next = s.find(delim, i) )
			tokens.push_back( ToView(s.substr(i, next-i)) );
		if( i<s.size() )
			tokens.push_back( ToView(s.substr(i)) );
		return tokens;
	}

	Ŧ Str::TryTo( str s, uint* pos, int base )ι->optional<T>{
		optional<T> y;
		try{
			y = static_cast<T>( std::stoull(s, pos, base) );
		}
		catch( const std::invalid_argument& )
		{}
		catch( const std::out_of_range& )
		{}
		return y;
	}
}
#undef Φ
#endif