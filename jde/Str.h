#pragma once
#include <charconv>
#include <codecvt>
#include <boost/algorithm/string/trim.hpp>
#include <span>
#include "Exception.h"
#include "Log.h"

#define var const auto
#define Φ Γ auto

namespace Jde::Str
{
	str Empty()ι;
	template<class T> using bsv = std::basic_string_view<char,T>;
	struct ci_traits : public std::char_traits<char>
	{
		Ω eq(char c1, char c2)ι{ return toupper(c1) == toupper(c2); }
		Ω ne(char c1, char c2)ι{ return toupper(c1) != toupper(c2); }
		Ω lt(char c1, char c2)ι{ return toupper(c1) <  toupper(c2); }
		Ω compare( const char* s1, const char* s2, uint n )ι->int
		{
			int y{};
			for( ; !y && n-- != 0; ++s1, ++s2 )
			{
				if( toupper(*s1) < toupper(*s2) ) y=-1;
				if( toupper(*s1) > toupper(*s2) ) y= 1;
			}
			return y;
		}
		Ω find( const char* s, uint n, char a )ι->const char*
		{
			while( n-- > 0 && toupper(*s) != toupper(a) )
				++s;
			return n==string::npos ? nullptr : s;
		}
	};
}
namespace Jde
{
	using iv = Str::bsv<Str::ci_traits>;
	using String = std::basic_string<char, Str::ci_traits>;
}
namespace Jde::Str
{
	Τ concept IsString = std::same_as<T, string> || std::same_as<T, String>;
	Τ concept IsView = std::same_as<T, sv> || std::same_as<T, iv>;
	Τ concept IsStringLike = IsString<T> || IsView<T>;
	Τ concept IsInsensitive = std::same_as<T, String> || std::same_as<T, iv>;
	Τ concept IsSensitive = std::same_as<T, string> || std::same_as<T, sv>;
}
namespace Jde
{
	template<class Y=sv, class X> α ToView( const X& x )ι->Y{ return Y{x.data(),x.size()}; }
	Ξ ToSV( iv x )ι->sv{ return ToView<sv,iv>( x ); }
	Ξ ToIV( sv x )ι->iv{ return ToView<iv,sv>( x ); }

	template<Str::IsInsensitive T> α ToStr( const T& x )ι->string{ return string{ x.data(), x.size() }; }
	template<Str::IsSensitive T> α ToIStr( const T& x )ι->String{ return String{ x.data(), x.size() }; }

	template<Str::IsView T=sv> α ToWString( T x )ι->std::wstring
	{
		#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes( x.data(), x.data()+x.size() );
	}
#define TT typename T::traits_type
#define $ std::basic_string<char, TT>
	template<Str::IsView T=sv> α ToString( const std::wstring& value )ι->$
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto s = converter.to_bytes( value );
		return ${ s.data(), s.size() };
	}


	Ξ operator<<( std::ostream& os, iv s )ι->std::ostream&{ os << ToSV(s); return os; }
	template<class Y, class X=sv>
	α Toε( const X& x, ELogLevel l=ELogLevel::Debug, SRCE )ε->Y
	{
		Y y;
		var e=std::from_chars( x.data(), x.data()+x.size(), y );
		if( e.ec!=std::errc{} )
			throw Jde::Exception{ sl, l, "Can't convert:  '{}'.  to '{}'.  ec='{}'"sv, x, "Jde::GetTypeName<T>()", (uint)e.ec };
		return y;
	}

	Ŧ To( sv value )ι->T
	{
		T v{};
		std::from_chars( value.data(), value.data()+value.size(), v );
		return v;
	}

	template<> Ξ To( sv x )ι->double
	{
		return stod( string{x} );
	}

	Ξ ToString( double x, uint8 precision )ι->string
	{
#ifdef _MSC_VER
		std::array<char, 128> b;
		auto [end, ec] = std::to_chars( b.data(), b.data() + b.size(), x, std::chars_format::fixed, (int)precision );
		return ec == std::errc{} ? string{} : std::string{ b.data(), end };
#else
		ostringstream os; os.precision( precision );
		os << x;
		return os.str();
#endif
	}

	constexpr iv operator "" _iv( const char* x, uint len )noexcept{ return iv(x, len); }
	Ŧ FromTraits( T x )->string{ return string{ x.data(), x.size() }; }
	struct Γ CIString : String
	{
		using base=String;
		CIString()ι{};
		CIString( base&& s )ι:base{ move(s) }{}
		CIString( sv sv )ι:base{sv.data(), sv.size()}{}
		CIString( str s )ι:base{s.data(), s.size()}{}
		CIString( const char* p, sv::size_type s )ι:base{p, s}{}
		α substr( uint i, uint c = npos )Ι->CIString{ return CIString{base::substr(i, c)}; }
		//uint find( sv sub, uint pos = 0 )Ι;
		Ŧ operator==( const T& s )Ι->bool{ return size()==s.size() && base::compare( 0, s.size(), s.data(), s.size() )==0; }
		α operator==( const char* psz )Ι->bool{ return size()==strlen(psz) && base::compare( 0, size(), psz, size() )==0; }
		friend std::ostream& operator<<( std::ostream &os, const CIString& obj )noexcept{ os << (string)obj; return os; }
		Ξ operator !=( sv s )Ι{ return size() == s.size() && base::compare(0, s.size(), s.data(), s.size())!=0; }
		Ξ operator !=( str s )Ι{ return *this!=sv{s}; }
		inline CIString& operator+=( sv s )ι
		{
			var l = size()+s.size();
			base::resize(l);
			std::copy( s.data(), s.data()+s.size(), data() );
			return *this;
		}
		Ξ operator[]( uint i )Ι{ return data()[i]; }
		operator string()Ι{ return string{data(), size()}; }
		operator iv()Ι{ return {data(), size()}; }
		explicit operator sv()Ι{ return {data(), size()}; }
	};
	namespace Str
	{
		using std::basic_string;
		template<class T=sv, class D=sv> α Split( bsv<TT> s, bsv<typename D::traits_type> delim, uint count, sv error, SRCE )ε->vector<bsv<TT>>;
		template<class T=sv, class D=sv> α Split( bsv<TT> s, bsv<typename D::traits_type> delim )ι->vector<bsv<TT>>;
		template<class X=sv, class Y=sv> α Split( bsv<typename X::traits_type> x, char delim=',', bool removeEmpty=false )ι->vector<bsv<typename Y::traits_type>>;

		Ŧ AddSeparators( T collection, sv separator, bool quote=false )ι->string;

		Ŧ AddCommas( T value, bool quote=false )ι{ return AddSeparators( value, ",", quote ); }

		Φ NextWord( sv x )ι->sv;

		Ŧ NextWordLocation( T x )ι->optional<tuple<T,uint>>;

		template<class T=string> α Replace( const T& source, bsv<TT> find, bsv<TT> replace )ι->$;
		Φ Replace( sv source, char find, char replace )ι->string;
		Φ ToLower( sv source )ι->string;
		Φ ToUpper( sv source )ι->string;

		Ŧ TryToFloat( const basic_string<T>& s )ι->float;
		optional<double> TryToDouble( str s )ι;
		template<class T=uint> α TryTo( str s, uint* pos = nullptr, int base = 10 )ι->optional<T>;

		template<class TEnum, class Collection> α FromEnum( const Collection& stringValues, TEnum value )ι->string;
		template<class TEnum, class TCollection, class TString> α ToEnum( const TCollection& s, TString text )ι->optional<TEnum>;

		[[nodiscard]]Ξ EndsWith( sv value, sv ending )ι{ return ending.size() > value.size() ? false : std::equal( ending.rbegin(), ending.rend(), value.rbegin() ); }
		[[nodiscard]]Ξ StartsWith( sv value, sv starting )ι{ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
		[[nodiscard]]Ξ StartsWithInsensitive( sv value, sv starting )ι->bool;

		template<IsString T> α LTrim_( T& s )->void;
		template<IsString T> α RTrim_( T& s )->void;
		template<IsString T> α Trim_( T& s )->void{ LTrim_(s); RTrim_(s); }
		[[nodiscard]] Ξ Trim_( string&& s )->string{ auto y{move(s)}; LTrim_(y); RTrim_(y); return y; }

#define TSV template<class T=sv> [[nodiscard]] α
#define X bsv<TT>
		TSV LTrim( X s )->X;
		TSV RTrim( X s )->X;
		TSV Trim( X s )->X{ return RTrim<T>( LTrim<T>(s) ); }

		TSV LTrim( X s, function<bool(char)> f )->X;
		TSV LTrim( X s, const vector<char>& tokens )->X;
		TSV RTrim( X s, function<bool(char)> f )->X;
		TSV RTrim( X s, const vector<char>& tokens )->X;
		TSV TrimPunct( X s, bool leaveTrailingPeriod=false )->X;

		TSV Trim( X s, X substring )ι->$;
		TSV Words( X x )ι->vector<X>;
		TSV StemmedWords( X x )ι->vector<$>;
		struct FindPhraseResult{ uint Start; uint StartNextWord; uint NextEntry{}; FindPhraseResult operator+(uint i)Ι{ return {Start+i, StartNextWord+i, NextEntry}; } };
		template<IsStringLike T> α FindPhrase( bsv<TT> x, const std::vector<T>& entries, bool stem=false )ι->optional<FindPhraseResult>;
		//[start,start of next word] of phrase index.  start of next word because of stemming
		TSV Pascal( X s )ι->$;
		TSV Camel( X s )ι->$;
#undef X
	}
	template<class T, class D> α Str::Split( bsv<TT> s, bsv<typename D::traits_type> delim, uint count, sv errorId, SL sl )ε->vector<bsv<TT>>
	{
		var y = Split( s, delim );
		if( y.size()!=count )
			throw Jde::Exception{ sl, ELogLevel::Error, "({})'{}' expected '{}' tokens vs parsed='{}'", errorId, s, count, y.size() };
		return y;
	}
	template<class T, class D> α Str::Split( bsv<TT> s_, bsv<typename D::traits_type> delim )ι->vector<bsv<TT>>
	{
		vector<T> tokens;
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

	template<class X, class Y> α Str::Split( bsv<typename X::traits_type> s, char delim, bool removeEmpty )ι->vector<bsv<typename Y::traits_type>>
	{
		vector<bsv<typename Y::traits_type>> results;
		for( uint fieldStart=0, fieldEnd;fieldStart<s.size();fieldStart = fieldEnd+1 )
		{
			fieldEnd = std::min( s.find_first_of(delim, fieldStart), s.size() );
			Y v{s.data()+fieldStart, fieldEnd-fieldStart };
			if( v.size() || !removeEmpty )
				results.push_back( v );
		}
		return results;
	}

	Ŧ Str::Replace( const T& x, bsv<TT> token, bsv<TT> replace )ι->$
	{
		$ y; y.reserve( x.size() ); uint iLast{ 0 };
		for( uint i{}; (i = x.find(token, i))!=string::npos; iLast = (i=i+token.length()) )
    	{
			y += x.substr( iLast, i-iLast );
			y += replace;
		}
		if( iLast<x.size() )
			y += x.substr( iLast, x.size()-iLast );

		return y;
	}

	Ŧ Str::LTrim( bsv<TT> s, function<bool(char)> f )->bsv<TT>
	{
		auto p = std::find_if( s.begin(), s.end(), f );
		return p==s.end() ? bsv<TT>{} : p==s.begin() ? s : bsv<TT>{ s.data()+std::distance(s.begin(),p), s.size()-std::distance(s.begin(),p) };
	}


	Ŧ Str::LTrim( bsv<TT> s, const vector<char>& tokens )->bsv<TT>
	{
		auto f = [&tokens](int ch){ return !std::isspace(ch) && std::find(tokens.begin(), tokens.end(), ch)==tokens.end(); };
		return LTrim<T>( s, f );
	}

	Ŧ Str::RTrim( bsv<TT> s, function<bool(char)> f )->bsv<TT>
	{
		bsv<TT> y;
		if( auto p = std::find_if(s.rbegin(), s.rend(), f); p!=s.rend() )
			y = p==s.rbegin() ? s : bsv<TT>{ s.data(), s.size()-std::distance(s.rbegin(), p) };
		return y;
	}
	Ŧ Str::RTrim( bsv<TT> s )->bsv<TT>
	{
		return RTrim<T>( s, [](int ch){return !std::isspace(ch);} );
	}
	Ŧ Str::RTrim( bsv<TT> s, const vector<char>& tokens )->bsv<TT>
	{
		auto f = [&tokens](int ch){ return !std::isspace(ch) && std::find(tokens.begin(), tokens.end(), ch)==tokens.end(); };
		return RTrim<T>( s, f );
	}
	Ŧ Str::TrimPunct( bsv<TT> s, bool leaveTrailingPeriod )->bsv<TT>
	{
		const vector<char> v{ '.',',',':','-','*','_' };
		auto y = RTrim<T>( LTrim<T>(s, v), v );
		if( uint i=leaveTrailingPeriod ? y.data()-s.data()+y.size() : s.size();  i<s.size() && s[i]=='.' )
			y = { y.data(), y.size()+1 };
		return y;
	}
	Ŧ Str::AddSeparators( T collection, sv separator, bool quote )ι->string
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

	Ŧ Str::TryToFloat( const basic_string<T>& token )ι->float
	{
		try
		{
			return std::stof( token );
		}
		catch( std::invalid_argument )
		{
			//TRACE( "Can't convert:  {}.  to float.  {}", token, e.what() );
			return std::nanf("");
		}
	}
	Ξ Str::TryToDouble( str s )ι->optional<double>
	{
		optional<double> v;
		try
		{
			v = std::stod( s );
		}
		catch( const std::invalid_argument& )
		{
			//TRACE( "Can't convert:  {}.  to float.  {}", s, e.what() );
		}
		return v;
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


	Ξ Str::StartsWithInsensitive( sv value, sv starting )ι->bool
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

	template<Str::IsString T> α Str::LTrim_( T& s )->void
	{
		boost::trim_left( s );
#ifdef _MSC_VER
		//auto w = Windows::ToWString( s );
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		auto w = converter.from_bytes( s.data(), s.data()+s.size() );
		boost::trim_left( w );
		string conv{ converter.to_bytes(w) };
		s = bsv<TT>{ conv.data(), conv.size() };
#else
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
		std::u32string utf32 = cvt.from_bytes( s.data(), s.data()+s.size() );
		::boost::algorithm::trim_left_if(utf32, boost::is_any_of(U"\x2000\x2001\x2002\x2003\x2004\x2005\x2006\x2007\x2009\x200A\x2028\x2029\x202f\x205f\x3000"));
		auto result = cvt.to_bytes( utf32 );
		s = T{ result.data(), result.size() };
#endif
	}
	//https://stackoverflow.com/questions/59589243/utf8-strings-boost-trim
	template<Str::IsString T> α Str::RTrim_( T& s )->void
	{
		boost::trim_right( s );
		std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
		std::u32string w = cvt.from_bytes( s.data(), s.data()+s.size() );
		::boost::algorithm::trim_right_if(w, boost::is_any_of(U"\x2000\x2001\x2002\x2003\x2004\x2005\x2006\x2007\x2009\x200A\x2028\x2029\x202f\x205f\x3000"));
		string conv{ cvt.to_bytes(w) };
		s = bsv<TT>{ conv.data(), conv.size() };
	}


	template<class TEnum, class TCollection, class TString>
	α Str::ToEnum( const TCollection& stringValues, TString text )ι->optional<TEnum>
	{
		typedef typename std::underlying_type<TEnum>::type T;
		T v = (T)std::distance( std::begin(stringValues), std::find(std::begin(stringValues), std::end(stringValues), text) );
		auto pResult = v<stringValues.size() ? optional<TEnum>((TEnum)v) : nullopt;
		if( !pResult )
		{
			uint v2;
			if( var e = std::from_chars(text.data(), text.data()+text.size(), v2); e.ec==std::errc() )
				pResult = v2<stringValues.size() ? optional<TEnum>((TEnum)v2) : nullopt;
		}
		return pResult;
	}
	template<class TEnum, class Collection>
	α Str::FromEnum( const Collection& stringValues, TEnum value )ι->string
	{
		return (uint)value<stringValues.size() ? string{ stringValues[(uint)value] } : std::to_string( (uint)value );
	}

	Ŧ Str::Trim( bsv<TT> s, bsv<TT> substring )ι->$
	{
		$ trimmed; uint current=0;
		for( uint i; (i = s.find(substring, current))!=string::npos; current = i+substring.size() )
			trimmed += bsv<TT>{ s.data()+current, i-current };

		if( current && current<s.size() )
			trimmed += s.substr( current, s.size()-current );
		return current ? trimmed : ${ s };
	}

	Ŧ Str::NextWordLocation( T x )ι->optional<tuple<T,uint>>
	{
		uint i = 0;
		for( ; i<x.size() && x[i]>0 && ::isspace(x[i]); ++i );//msvc asserts if ch<0
		uint e=i;
		for( ;e<x.size() && (x[e]<0 || !::isspace(x[e])); ++e );
		return e==i ? nullopt : optional<tuple<T,uint>>{ make_tuple(x.substr(i, e-i), e) };
	}

	namespace Internal
	{
		Ŧ PorterStemmer( T s )->$//TODO get library or old code.
		{
			return ${ s.ends_with('s') || s.ends_with('S') ? s.substr(0,s.size()-1) : s };
		}

		template<Str::IsStringLike T, Str::IsStringLike TCriteria> α FindPhraseT( tuple<vector<T>,vector<uint>> x, const std::vector<TCriteria>& criteria )ι->optional<Str::FindPhraseResult>
		{
			var& words = get<0>(x);
			if( criteria.empty() || !words.size() )
				return nullopt;

			var& locations = get<1>( x );
			T firstWord{ criteria.front().data(), criteria.front().size() }; optional<Str::FindPhraseResult> y;
			for( auto p=words.begin(); (p=std::find(p, words.end(), firstWord))!=words.end(); ++p )
			{
				var iStartWord{ (uint)std::distance(words.begin(), p) }; uint i = iStartWord;
				bool equal = true;
				uint j = 1;
				for( ; equal && i<words.size()-1 && j<criteria.size(); ++j )
					equal = words[++i]==T{ criteria[j].data(), criteria[j].size() };
				if( var iStart = locations[iStartWord]; /*equal && (*/!y || iStart<y->Start || j>y->NextEntry/*)*/ )
				{
					y = Str::FindPhraseResult{ iStart, locations[std::min(equal ? i : i-1, locations.size()-1)], equal ? j : j-1 };
					if( y->NextEntry==criteria.size() )
						break;
				}
			}
			return y;
		}

		TSV GetChar( Str::bsv<TT> x, uint& i )ι->wchar_t
		{
			wchar_t ch = i<x.size() ? x[i] : '\0';
			if( ch==L'\xffe2' && i+2<x.size() )
			{
				ch =  x[++i];
				ch = ch << 8;
				wchar_t lo = x[++i] & 0xff; //lo &= 0xff;
				ch += lo;
			}
			return ch;
		};
#undef TSV

		template<class T=sv, bool TStem=false> α WordsLocation( Str::bsv<TT> x )ι->tuple<vector<T>,vector<uint>> //[words, endIndex]
		{
			tuple<vector<T>,vector<uint>> y;
			auto isSeparator = []( wchar_t ch )ι->bool{ return ::isspace(ch) || ch=='.' || ch==';' || ch==':' || ch=='(' || ch==')' || ch=='/' || ch==L'\x80af' || ch==L'\x8093'; };
			for( uint iStart{0}, iEnd; iStart<x.size(); iStart=iEnd )
			{
				wchar_t ch=GetChar<T>( x, iStart );
				for( ; isSeparator(ch); ch=GetChar<T>( x, ++iStart) );
				iEnd=iStart;
				for( ; ch!='\0' && !isSeparator(ch); ch=GetChar<T>(x, ++iEnd) );
				if( ch>0xff )
					iEnd-=2;
				if( var length{iEnd-iStart}; length )
				{
					T v{ x.substr(iStart, length) };
					if constexpr( TStem )
						get<0>( y ).push_back( PorterStemmer(Str::TrimPunct<T>(v)) );
					else
						get<0>( y ).push_back( v );

					get<1>( y ).push_back( iStart+v.size() );
				}
				else if( iStart!=x.size() )
					++iEnd;
			}
			return y;
		}
	}

	Ŧ Str::LTrim( Str::bsv<TT> s )->Str::bsv<TT>
	{
		uint i=0; wchar_t ch;
		for( ch = Internal::GetChar<T>( s, i );
			std::isspace(ch) || ch==L'\x80af' || ch==L'\x8093';
			ch = Internal::GetChar<T>(s, ++i) );
		if( ch>0xff && i>1 )
			i-=2;
		return i ? bsv<TT>{ s.data()+i, s.size()-i } : s;
		//return s;
	}

	Ŧ Str::Words( bsv<TT> x )ι->vector<bsv<TT>>{ return get<0>( Internal::WordsLocation<T>(x) ); }

	Ŧ Str::StemmedWords( bsv<TT> x )ι->vector<$>
	{
		return get<0>( Internal::WordsLocation<$,true>(x) );
	}

	template<Str::IsStringLike T> α Str::FindPhrase( bsv<TT> x, const std::vector<T>& criteria, bool stem )ι->optional<FindPhraseResult>
	{
		return stem
			? Internal::FindPhraseT<$>( Internal::WordsLocation<$,true>(bsv<TT>{x.data(), x.size()}), criteria )
			: Internal::FindPhraseT( Internal::WordsLocation<$>( bsv<TT>{x.data(), x.size()} ), criteria );
	}

	Ŧ Str::Pascal( bsv<TT> x )ι->$
	{
		$ y{ x };
		if( y.size() )
			y[0] = std::toupper( x[0] );
		return y;
	}
	Ŧ Str::Camel( bsv<TT> x )ι->$
	{
		$ y{ x };
		if( y.size() )
			y[0] = std::tolower( x[0] );
		return y;
	}
}

template<> struct fmt::formatter<Jde::iv>
{
	constexpr α parse( fmt::format_parse_context& ctx )->decltype(ctx.begin()){ return ctx.end(); }
	Ŧ format( Jde::iv x, T& ctx )->decltype(ctx.out()){ return format_to( ctx.out(), "{}", Jde::ToSV(x) ); }
};

template<> struct fmt::formatter<Jde::String>
{
	constexpr α parse( fmt::format_parse_context& ctx )->decltype(ctx.begin()){ return ctx.end(); }
	Ŧ format( Jde::iv x, T& ctx )->decltype(ctx.out()){ return format_to( ctx.out(), "{}", Jde::ToStr(x) ); }
};

#undef var
#undef Φ
#undef $
#undef TT