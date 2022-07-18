#pragma once
#include <sstream>
#include "../TypeDefs.h"

//https://stackoverflow.com/questions/21806561/concatenating-strings-and-numbers-in-variadic-template-function
namespace Jde::ToVec
{
	inline void Append( vector<string>& /*values*/ ){}

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t );

	template<typename... Tail>
	void Append( vector<string>& values, string&& h, Tail&&... t )
	{
		values.push_back( h );
		return Apend( values, std::forward<Tail>(t)... );
	}

	Ŧ ToStringT( const T& x )->string
	{
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

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t )
	{
		values.push_back( ToStringT(std::forward<Head>(h)) );
		return Append( values, std::forward<Tail>(t)... );
	}
}