#pragma once
//#include <string>
//#include <sstream>
//#include <list>
//#include <functional>
//#include <cctype>
#include <charconv>
#include <span>
#include "Exception.h"
#include "Log.h"

#define var const auto
#define Φ Γ auto
namespace Jde
{
	namespace Str
	{
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
	using iv = Str::bsv<Str::ci_traits>;
	template<class Y=sv, class X> α ToView( const X& x )ι->Y{ return Y{x.data(),x.size()}; }
	Ξ ToSV( iv x )ι->sv{ return ToView<sv,iv>( x ); }
	Ξ ToIV( sv x )ι->iv{ return ToView<iv,sv>( x ); }
/*	ⓣ Toε( sv value, SRCE )ε->T
	{
		T v;
		var e=std::from_chars( value.data(), value.data()+value.size(), v );
		if( e.ec!=std::errc{} ) 
			throw Jde::Exception{ sl, ELogLevel::Debug, "Can't convert:  '{}'.  to '{}'.  ec='{}'"sv, value, "Jde::GetTypeName<T>()", (uint)e.ec };
		return v;
	}
*/
	template<class Y, class X=sv>
	α Toε( const X& x, ELogLevel l=ELogLevel::Debug, SRCE )ε->Y
	{
		Y y;
		var e=std::from_chars( x.data(), x.data()+x.size(), y );
		if( e.ec!=std::errc{} ) 
			throw Jde::Exception{ sl, l, "Can't convert:  '{}'.  to '{}'.  ec='{}'"sv, x, "Jde::GetTypeName<T>()", (uint)e.ec };
		return y;
	}

	ⓣ To( sv value )ι->T
	{
		T v{};
		std::from_chars( value.data(), value.data()+value.size(), v );
		return v;
	}

	template<> Ξ To( sv x )ι->double
	{
		return stod( string{x} );
	}


	constexpr iv operator "" _iv( const char* x, uint len )noexcept{ return iv(x, len); }
	ⓣ FromTraits( T x )->string{ return string{ x.data(), x.size() }; }
	using String = std::basic_string<char, Str::ci_traits>;
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
		ⓣ operator==( const T& s )Ι->bool{ return size()==s.size() && base::compare( 0, s.size(), s.data(), s.size() )==0; }
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
#define TT typename T::traits_type
#define $ std::basic_string<char, TT>
	namespace Str
	{
		Τ concept IsString = std::same_as<T, string> || std::same_as<T, String>;
		Τ concept IsView = std::same_as<T, sv> || std::same_as<T, iv>;
		using std::basic_string;
		template<class T=sv, class D=sv> α Split( bsv<TT> s, bsv<typename D::traits_type> delim )ι->vector<bsv<TT>>;
		template<class X=sv, class Y=sv> α Split( bsv<typename X::traits_type> x, char delim=',', bool removeEmpty=false )ι->vector<bsv<typename Y::traits_type>>;

		ⓣ AddSeparators( T collection, sv separator, bool quote=false )ι->string;

		ⓣ AddCommas( T value, bool quote=false )ι{ return AddSeparators( value, ",", quote ); }

		Φ NextWord( sv x )ι->sv;

		ⓣ NextWordLocation( T x )ι->optional<tuple<T,uint>>;

		template<class T=string> α Replace( const T& source, bsv<TT> find, bsv<TT> replace )ι->$;
		Φ Replace( sv source, char find, char replace )ι->string;
		Φ ToLower( sv source )ι->string;
		Φ ToUpper( sv source )ι->string;

		ⓣ TryToFloat( const basic_string<T>& s )ι->float;
		optional<double> TryToDouble( str s )ι;

		template<class TEnum, class Collection> α FromEnum( const Collection& stringValues, TEnum value )ι->string;
		template<class TEnum, class TCollection, class TString> α ToEnum( const TCollection& s, TString text )ι->optional<TEnum>;

		[[nodiscard]]Ξ EndsWith( sv value, sv ending )ι{ return ending.size() > value.size() ? false : std::equal( ending.rbegin(), ending.rend(), value.rbegin() ); }
		[[nodiscard]]Ξ StartsWith( sv value, sv starting )ι{ return starting.size() > value.size() ? false : std::equal( starting.begin(), starting.end(), value.begin() ); }
		[[nodiscard]]Ξ StartsWithInsensitive( sv value, sv starting )ι->bool;

		
		Φ LTrim_( string& s )->void;
		Φ RTrim_( string& s )->void;
		Ξ Trim_( string& s )->void{ LTrim_(s); RTrim_(s); }
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
		TSV TrimPunct( X s )->X{ const vector<char> v{'.',',',':','-','*'}; return RTrim<T>( LTrim<T>(s, v), v ); }

		TSV Trim( X s, X substring )ι->$;
		TSV Words( X x )ι->vector<X>;
		Φ StemmedWords( sv x )ι->vector<string>;
		ⓣ FindPhrase( T x, const std::span<X>& entries, bool stem=false )ι->optional<tuple<uint,uint>>;
#undef TSV
#undef X
	};
	/*
	struct StringCompare
	{
   	α operator()( str a, str b )const ι{ return a<b; }
   };*/
	template<class T, class D> α Str::Split( bsv<TT> s_, bsv<typename D::traits_type> delim )ι->vector<bsv<TT>>
	{
		vector<T> tokens;
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
		for( uint fieldStart=0, iField=0, fieldEnd;fieldStart<s.size();++iField, fieldStart = fieldEnd+1 )
		{
			fieldEnd = std::min( s.find_first_of(delim, fieldStart), s.size() );
			Y v{s.data()+fieldStart, fieldEnd-fieldStart };
			if( v.size() || !removeEmpty )
				results.push_back( v );
		}
		return results;
	}

	ⓣ Str::Replace( const T& x, bsv<TT> token, bsv<TT> replace )ι->$
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

	ⓣ Str::LTrim( bsv<TT> s, function<bool(char)> f )->bsv<TT>
	{
		auto p = std::find_if( s.begin(), s.end(), f );
		return p==s.end() ? bsv<TT>{} : p==s.begin() ? s : bsv<TT>{ s.data()+std::distance(s.begin(),p), s.size()-std::distance(s.begin(),p) }; 
	}
	ⓣ Str::LTrim( bsv<TT> s )->bsv<TT>
	{ 
		auto f = [](int ch){ return !std::isspace(ch); };
		return LTrim<T>( s, f );
	}

	ⓣ Str::LTrim( bsv<TT> s, const vector<char>& tokens )->bsv<TT>
	{
		auto f = [&tokens](int ch){ return !std::isspace(ch) && std::find(tokens.begin(), tokens.end(), ch)==tokens.end(); };
		return LTrim<T>( s, f );
	}
	
	ⓣ Str::RTrim( bsv<TT> s, function<bool(char)> f )->bsv<TT>
	{
		bsv<TT> y;
		if( auto p = std::find_if(s.rbegin(), s.rend(), f); p!=s.rend() )
			y = p==s.rbegin() ? s : bsv<TT>{ s.data(), s.size()-std::distance(s.rbegin(), p) };
		return y;
	}
	ⓣ Str::RTrim( bsv<TT> s )->bsv<TT>
	{
		return RTrim<T>( s, [](int ch){return !std::isspace(ch);} );
		//if( auto p = std::find_if(s.rbegin(), s.rend(), [](int ch){return !std::isspace(ch);}); p!=s.rend() )
		//{
		//	if( p==s.rbegin() )
		//		y = s;
		//	else
		//	{
		//		var size = std::distance( s.rbegin(), p );
		//		y = { s.data(), s.size()-size };
		//	}
		//}
		//return y;
	}
	ⓣ Str::RTrim( bsv<TT> s, const vector<char>& tokens )->bsv<TT>
	{
		auto f = [&tokens](int ch){ return !std::isspace(ch) && std::find(tokens.begin(), tokens.end(), ch)==tokens.end(); };
		return RTrim<T>( s, f );
	}

	ⓣ Str::AddSeparators( T collection, sv separator, bool quote )ι->string
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

/*	ⓣ Str::Split( const basic_string<T> &s, T delim/ *=T{','}* / )ι->vector<std::basic_string<T>>
	{
		vector<basic_string<T>> tokens;
		basic_string<T> token;
		std::basic_istringstream<T> tokenStream(s);
		while( std::getline(tokenStream, token, delim) )
			tokens.push_back(token);

		return tokens;
	}
*/
	ⓣ Str::TryToFloat( const basic_string<T>& token )ι->float
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
	Ξ Str::TryToDouble( str s )ι->optional<double>
	{
		optional<double> v;
		try
		{
			v = std::stod( s );
		}
		catch( const std::invalid_argument& e )
		{
			TRACE( "Can't convert:  {}.  to float.  {}", s, e.what() );
		}
		return v;
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

	ⓣ Str::Trim( bsv<TT> s, bsv<TT> substring )ι->$
	{
		$ trimmed; uint current=0;
		for( uint i; (i = s.find(substring, current))!=string::npos; current = i+substring.size() )
			trimmed += bsv<TT>{ s.data()+current, i-current };

		if( current && current<s.size() )
			trimmed += s.substr( current, s.size()-current );
		return current ? trimmed : ${ s };
	}

	ⓣ Str::NextWordLocation( T x )ι->optional<tuple<T,uint>>
	{
		uint i = 0;
		for( ; i<x.size() && x[i]>0 && ::isspace(x[i]); ++i );//msvc asserts if ch<0
		uint e=i;
		for( ;e<x.size() && (x[e]<0 || !::isspace(x[e])); ++e );
		return e==i ? nullopt : optional<tuple<T,uint>>{ make_tuple(x.substr(i, e-i), e) };
	}
	
	namespace Internal
	{
		ⓣ PorterStemmer( T s )->$//TODO get library or old code.
		{
			return ${ s.ends_with('s') || s.ends_with('S') ? s.substr(0,s.size()-1) : s };
		}

		template<class T=sv> α FindPhraseT( tuple<vector<T>,vector<uint>> x, const std::span<Str::bsv<TT>>& criteria )ι->optional<tuple<uint,uint>>//[start,start of next word] of phrase index.  start of next word because of stemming
		{
			var& words = get<0>(x);
			if( criteria.empty() || !words.size() )
				return nullopt;

			const vector<T> w{ words.begin(), words.end() }; optional<tuple<uint,uint>> y;
			T firstWord{ criteria.front().data(), criteria.front().size() };
			for( auto p=w.begin(); (p=std::find(p, w.end(), firstWord))!=w.end(); ++p )
			{
				const uint iStartWord{ (uint)std::distance(w.begin(), p) }; uint i = iStartWord;
				bool equal = w.size()-iStartWord>=criteria.size();
				for( uint j=1; equal && j<criteria.size(); ++j )
					equal = w[++i]==T{ criteria[j].data(), criteria[j].size() };
				var& locations = get<1>( x );
				var iStart = locations[iStartWord];
				if( equal && (!y || iStart<get<0>(*y)) )
					y = make_tuple( iStart, i+1==locations.size() ? locations[i]+criteria.back().size() : locations[i+1]  );
			}
			return y;
		}
		template<class T=sv, bool TStem=false> α WordsLocation( Str::bsv<TT> x )ι->tuple<vector<T>,vector<uint>>
		{
			tuple<vector<T>,vector<uint>> y;
			auto isSeparator = []( char ch )ι->bool{ return ch>0 && (::isspace(ch) || ch=='.' || ch==':' || ch=='(' || ch==')'); };//msvc asserts if ch<0
			for( uint iStart{0}, iEnd; iStart<x.size(); iStart=iEnd )
			{
				auto ch=x[iStart];
	#define INCR(i) ch= ++i<x.size() ? x[i] : '\0'
				for( ; isSeparator(ch); INCR(iStart) );
				iEnd=iStart;
				for( ; ch!='\0' && !isSeparator(ch); INCR(iEnd) );
				if( var length{iEnd-iStart}; length )
				{
					T v{ x.substr(iStart, length) };
					if constexpr( TStem )
						get<0>( y ).push_back( PorterStemmer(v) );
					else
						get<0>( y ).push_back( v );

					get<1>( y ).push_back( iStart );
				}
			}
			return y;
		}
	}

	ⓣ Str::Words( bsv<TT> x )ι->vector<bsv<TT>>{ return get<0>( Internal::WordsLocation<T>(x) ); }

	ⓣ Str::FindPhrase( T x, const std::span<bsv<TT>>& criteria, bool stem )ι->optional<tuple<uint,uint>>
	{
		return stem 
			? Internal::FindPhraseT<$>( Internal::WordsLocation<$,true>(bsv<TT>{x.data(), x.size()}), criteria )
			: Internal::FindPhraseT( Internal::WordsLocation<$>( bsv<TT>{x.data(), x.size()} ), criteria );
	}

/*	struct Parser
	{
		Parser( sv text, uint i_=0, uint iLine=1 )ι:Text{text}, i{i_},_line{iLine}{}
		Φ Next( sv x )ι->sv;

		α Index()Ι{ return i;}
		α Line()Ι->uint{ return _line;}
		sv Text;
	protected:
		uint i;
		uint _line;
		sv _peekValue;
	};

	struct TokenParser : Parser
	{
		TokenParser( sv text, TokenParser* p, uint i_=0, uint iLine=1 )ι: Parser{text, i_, iLine==1 && p ? p->Line() : iLine}, Tokens{p ? p->Tokens : vector<sv>{}}{}
		TokenParser( sv text, vector<sv> tokens )ι:Parser{text}, Tokens{move(tokens)}{}
		Φ Next()ι->sv;
		Φ Next( const vector<sv>& tokens, bool dbg=false )ι->sv;
		Φ Next( char end )ι->sv;
		α Peek()ι->sv{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		α SetText( string x, uint index )ι{ _text = mu<string>( move(x) ); Text=*_text; i=index; }
		const vector<sv> Tokens;
	private:
		up<string> _text;
	};
	*/
#undef var
#undef Φ
#undef $
#undef TT
}