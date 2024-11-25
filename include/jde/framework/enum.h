#pragma once
#include <charconv>

#define $ template<IsEnum T> constexpr α
#define let const auto
namespace Jde{
	Τ concept IsEnum = std::is_enum_v<T>;
	$ underlying( T a )ι{ return std::to_underlying<T>(a); };
	$ operator|( T a, T b )ι->T{ return (T)( underlying(a)|underlying(b) ); };
	$ operator&( T a, T b )ι->T{ return (T)(underlying(a)&underlying(b)); };
	$ operator<( T a, T b )ι->bool{ return underlying(a)<underlying(b); };
	$ operator~( T a )ι{ return (T)( ~underlying(a) ); }
	$ operator|=( T& a, T b ){ return a = a | b; }
	$ operator!( T x ){ return underlying(x)!=0; }
	$ empty( T a )ι->bool{ return underlying(a)==0; };

	template<IsEnum TEnum, class Collection>
	α FromEnum( const Collection& stringValues, TEnum value )ι->string{
		return (uint)value<stringValues.size() ? string{ stringValues[(uint)value] } : std::to_string( (uint)value );
	}

	template<IsEnum Flag, class Collection>
	α FromEnumFlag( const Collection& stringValues, Flag flag )ι->string{
		let uflag = underlying( flag );
		if( uflag==0 )
			return stringValues.size() ? string{stringValues[0]} : "0";
		uint remaining = uflag;
		string flags;
		for( uint i=0; i+1<stringValues.size(); ++i ){
			if( uint current = (1ul<<i); uflag & current ){
				flags += stringValues[i+1];
				remaining -= current;
				flags.push_back( ',' );
			}
		}
		if( flags.size() )
			flags.pop_back();
		return flags;
	}

	template<IsEnum TEnum, class TCollection, class TString>
	α ToEnum( const TCollection& stringValues, TString text )ι->optional<TEnum>{
		using T = typename std::underlying_type<TEnum>::type;
		T v = (T)std::distance( std::begin(stringValues), std::find(std::begin(stringValues), std::end(stringValues), text) );
		auto pResult = (uint)v<stringValues.size() ? optional<TEnum>((TEnum)v) : nullopt;
		if( !pResult ){
			uint v2;
			if( let e = std::from_chars(text.data(), text.data()+text.size(), v2); e.ec==std::errc() )
				pResult = v2<stringValues.size() ? optional<TEnum>((TEnum)v2) : nullopt;
		}
		return pResult;
	}

	template<IsEnum TEnum, class TCollection, class TString>
	α ToFlag( const TCollection& stringValues, TString text )ι->optional<TEnum>{
		using T = typename std::underlying_type<TEnum>::type;
		let index = std::distance( std::begin(stringValues), std::find(std::begin(stringValues), std::end(stringValues), text) );
		return index && index<stringValues.size()
			? (TEnum)( 1<<(index-1) )
			: optional<TEnum>{};
	}
}
#undef $
#undef let