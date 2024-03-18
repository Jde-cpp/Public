#pragma once
#ifndef TO_VEC_H //gcc pragma once is not supported
#define TO_VEC_H
#include <sstream>
#include "../TypeDefs.h"

//https://stackoverflow.com/questions/21806561/concatenating-strings-and-numbers-in-variadic-template-function
namespace Jde::ToVec
{
	inline void Append( vector<string>& /*values*/ ){}

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t )ι;

	template<typename... Tail>
	void Append( vector<string>& values, string&& h, Tail&&... t )ι{
		values.push_back( h );
		return Apend( values, std::forward<Tail>(t)... );
	}

	template<class T> inline α ToStringT( const T& x )ι->string{
		constexpr bool StringConcept = requires(const T& t) { t.data(); t.size(); };
		if constexpr( StringConcept )
		{
			return string{ x.data(), x.size() };
		}
		else
		{
			ostringstream os;
			os << x;
			return os.str();
		}
	}

	Ξ FormatVectorArgs( sv fmt, const vector<string>& args )ε{
		return std::accumulate(
			std::begin( args ),
			std::end( args ),
			string{ fmt },
			[](sv toFmt, str arg){ return fmt::vformat( toFmt, fmt::make_format_args(arg) ); }
		);
	}	

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t )ι{
		values.push_back( ToStringT(std::forward<Head>(h)) );
		return Append( values, std::forward<Tail>(t)... );
	}
}
#endif