#pragma once
#include <string>
#include <sstream>
#include <list>
#include <functional>
#include <cctype>
#include <charconv>
#include "Exception.h"
#include "Log.h"


namespace Jde
{
	#define 🚪 JDE_NATIVE_VISIBILITY auto

	struct ci_char_traits : public std::char_traits<char>
	{
		static bool eq(char c1, char c2)noexcept{ return toupper(c1) == toupper(c2); }
		static bool ne(char c1, char c2)noexcept{ return toupper(c1) != toupper(c2); }
		static bool lt(char c1, char c2)noexcept{ return toupper(c1) <  toupper(c2); }
		JDE_NATIVE_VISIBILITY static int compare( const char* s1, const char* s2, size_t n )noexcept;
		JDE_NATIVE_VISIBILITY static const char* find( const char* s, size_t n, char a )noexcept;
	};
	#define var const auto
	struct JDE_NATIVE_VISIBILITY CIString : public std::basic_string<char, ci_char_traits>
	{
		using base=basic_string<char, ci_char_traits>;
		CIString()noexcept{};
		CIString( sv sv )noexcept:base{sv.data(), sv.size()}{}
		CIString( str s )noexcept:base{s.data(), s.size()}{}
		CIString( const char* p, sv::size_type s )noexcept:base{p, s}{}
		uint find( sv sub, uint pos = 0 )const noexcept;
		template<class T> bool operator==( const T& s )const noexcept{ return size()==s.size() && base::compare( 0, s.size(), s.data(), s.size() )==0; }
		bool operator==( const char* psz )const noexcept{ return size()==strlen(psz) && base::compare( 0, size(), psz, size() )==0; }
		friend std::ostream& operator<<( std::ostream &os, const CIString& obj )noexcept{ os << (string)obj; return os; }
		inline bool operator !=( sv s )const noexcept{ return size() == s.size() && base::compare(0, s.size(), s.data(), s.size())!=0; }
		inline bool operator !=( str s )const noexcept{ return *this!=sv{s}; }
		inline CIString& operator+=( sv s )noexcept
		{
			var l = size()+s.size();
			base::resize(l);
			std::copy( s.data(), s.data()+s.size(), data() );
			return *this;
		}
		inline char operator[]( uint i )const noexcept{ return data()[i]; }
		operator string()const noexcept{ return string{data(), size()}; }
		operator sv()const noexcept{ return sv{data(), size()}; }
	};

	namespace Str
	{
		using std::basic_string;
		ⓣ RTrim(basic_string<T> &s)noexcept->basic_string<T>;

		ⓣ Split( const basic_string<T> &s, T delim=T{','} )->vector<basic_string<T>>;

		🚪 Split( sv s, sv delim )->vector<sv>;
		🚪 Split( sv text, const CIString& delim )->vector<sv>;
		🚪 Split( sv s, char delim=',', uint estCnt=0 )->vector<sv>;

		template<typename T>
		string AddSeparators( T collection, sv separator, bool quote=false );

		template<typename T>
		string AddCommas( T value, bool quote=false ){ return AddSeparators( value, ",", quote ); }

		🚪 NextWord( sv x )noexcept->sv;

		std::wstring PorterStemmer(const std::wstring &s);

		🚪 Replace( sv source, sv find, sv replace )noexcept->string;
		🚪 Replace( sv source, char find, char replace )noexcept->string;
		🚪 ToLower( sv source )noexcept->string;
		JDE_NATIVE_VISIBILITY string ToUpper( sv source )noexcept;

		ⓣ TryTo( sv value )noexcept->optional<T>;
		ⓣ To( sv value )noexcept(false)->T;
		ⓣ TryToFloat( const basic_string<T>& s )noexcept->float;
		optional<double> TryToDouble( str s )noexcept;

		template<typename Enum, typename Collection>
		sv FromEnum( const Collection& s, Enum value )noexcept;
		template<class TEnum, class TCollection, class TString> α ToEnum( const TCollection& s, TString text )noexcept->optional<TEnum>;

		[[nodiscard]]inline bool EndsWith( sv value, sv ending ){ return ending.size() > value.size() ? false : std::equal( ending.rbegin(), ending.rend(), value.rbegin() ); }
		[[nodiscard]]inline bool StartsWith( sv value, sv starting ){ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
		[[nodiscard]]inline bool StartsWithInsensitive( sv value, sv starting );

		inline void LTrim( string& s ){ s.erase( s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {return !std::isspace(ch); }) ); }
		inline void RTrim( string& s ){ s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch) {return !std::isspace(ch);}).base(), s.end() ); }
		inline void Trim( string& s ){ LTrim(s); RTrim(s); }
		inline string Trim( str s ){ auto y{s}; LTrim(y); RTrim(y); return y; }

		template<typename T> T Trim( const T& s, sv substring )noexcept;

	};

	struct StringCompare
	{
   	bool operator()( str a, str b )const noexcept{ return a<b; }
   };

	ⓣ Str::RTrim(basic_string<T> &s)noexcept->basic_string<T>
	{
#ifdef _MSC_VER
		for( uint i=s.size()-1; i>=0; --i )
		{
			if( isspace(s[i]) )
				s.erase( *s.rbegin() );
			else
				break;
		}
#else
		s.erase( std::find_if(s.rbegin(), s.rend(), [](int ch){return !std::isspace(ch);}).base(), s.end() );
#endif
		return s;
	}

	template<typename T>
	string Str::AddSeparators( T collection, sv separator, bool quote )
	{
		ostringstream os;
		auto first = true;
		for( const auto& item : collection )
		{
			if( first )
				first = false;
			else
				os << separator;
			if( quote )
				os << "'" << item << "'";
			else
				os << item;
		}
		return os.str();
	}

	template<typename T>
	std::vector<std::basic_string<T>> Str::Split( const basic_string<T> &s, T delim/*=T{','}*/ )
	{
		vector<basic_string<T>> tokens;
		basic_string<T> token;
		std::basic_istringstream<T> tokenStream(s);
		while( std::getline(tokenStream, token, delim) )
			tokens.push_back(token);

		return tokens;
	}

	template<typename T>
	float Str::TryToFloat( const basic_string<T>& token )noexcept
	{
		try
		{
			return std::stof( token );
		}
		catch(std::invalid_argument e)
		{
			TRACE( "Can't convert:  {}.  to float.  {}", token, e.what() );
			return std::nanf("");
		}
	}
	inline optional<double> Str::TryToDouble( str s )noexcept
	{
		optional<double> v;
		try
		{
			v = std::stod( s );
		}
		catch(std::invalid_argument e)
		{
			TRACE( "Can't convert:  {}.  to float.  {}"sv, s, e.what() );
		}
		return v;
	}
	template<typename T>
	optional<T> Str::TryTo( sv value )noexcept
	{
		optional<T> v;
		Try( [&v, value]{v=To<T>( value );} );
		return v;
	}
	template<typename T>
	T Str::To( sv value )
	{
		T v;
		var e=std::from_chars( value.data(), value.data()+value.size(), v );
		THROW_IF( e.ec!=std::errc(), "Can't convert:  '{}'.  to '{}'.  ec='{}'"sv, value, "Jde::GetTypeName<T>()", (uint)e.ec);
		return v;
	}
	inline bool Str::StartsWithInsensitive( sv value, sv starting )
	{
		bool equal = starting.size() <= value.size();
		if( equal )
		{
			for( sv::size_type i=0; i<starting.size(); ++i )
			{
				equal = ::toupper(starting[i])==::toupper(value[i]);
				if( !equal )
					break;
			}
		}
		return equal;
	}

	template<class TEnum, class TCollection, class TString>
	α Str::ToEnum( const TCollection& stringValues, TString text )noexcept->optional<TEnum>
	{
		typedef typename std::underlying_type<TEnum>::type T;
		T v = (T)std::distance( std::begin(stringValues), std::find(std::begin(stringValues), std::end(stringValues), text) );
		auto pResult = v<stringValues.size() ? optional<TEnum>((TEnum)v) : nullopt;
		if( !pResult )
		{
			if( auto p = TryTo<T>(text); p )
				pResult = *p<stringValues.size() ? optional<TEnum>((TEnum)*p) : nullopt;
		}
		return pResult;
	}
	template<class TEnum, class Collection>
	sv Str::FromEnum( const Collection& stringValues, TEnum value )noexcept
	{
		return static_cast<uint>(value)<stringValues.size() ? stringValues[(uint)value] : sv{};
	}

	template<typename T> T Str::Trim( const T& s, sv substring )noexcept
	{
		T os; uint i, current=0;
		while( (i = s.find(substring, current))!=string::npos )
		{
			os += sv{ s.data()+current, i-current };
			current = i+substring.size();
		}
		if( current && current<s.size() )
			os+=sv{ s.data()+current, s.size()-current };
		return current ? os : s;
	}

#undef var
#undef 🚪
}
