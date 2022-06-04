#pragma once
#include "../Exception.h"

using Decimal=unsigned long long;
namespace Jde
{
	Ξ ToDecimal( double v )ι->::Decimal{ unsigned int _; ::Decimal y = binary64_to_bid64( v, 0, &_ ); return y; }
	Ξ ToDouble( ::Decimal v )ι->double
	{
		unsigned int _;
		double y = bid64_to_binary64( v, 0, &_ );
		auto _2 = ToDecimal( y );
		return y;
	}
	Ξ ToString( ::Decimal v )ι->string{ char y[64]; unsigned int _; bid64_to_string(y, v, &_ ); return y; }
	Ξ Subtract( ::Decimal x1, ::Decimal x2 )ι->::Decimal{ unsigned int _; return bid64_sub(x1, x2, 0, &_); }
	Ξ Add( ::Decimal x1, ::Decimal x2 )ι->::Decimal{ unsigned int _; return bid64_add(x1, x2, 0, &_); }
	Ξ Divide( ::Decimal x1, ::Decimal x2)ι->::Decimal{ unsigned int _; return bid64_div(x1, x2, 0, &_); }
	Ξ Multiply( ::Decimal x1, ::Decimal x2 )ι->::Decimal{ unsigned int _; return bid64_mul(x1, x2, 0, &_); }

	struct Decimal
	{
		Decimal()ι:_value{0}{}
		Decimal( ::Decimal x )ι:_value{x}{}
		Decimal( double x )ι:_value{ ToDecimal(x) }{}
		//α ToDouble()Ι->double{ return ToDouble(*_value); }
		explicit operator double()Ι
		{
			auto y = ToDouble(_value);
			return y;
		}
		explicit operator ::Decimal()Ι{ return _value; }
		friend auto operator<=>( const Decimal&, const Decimal& )ι = default;
		friend α operator-( Decimal a, Decimal b )ι->Decimal{ return Subtract( a._value,b._value ); }
		friend α operator+( Decimal a, Decimal b )ι->Decimal{ return Add( a._value,b._value ); }
		friend α operator/( Decimal a, Decimal b )ε->Decimal{ CHECK(b._value!=ToDecimal(0)); return Divide( a._value,b._value ); }
		friend α operator*( Decimal a, Decimal b )ι->Decimal{ return Multiply( a._value,b._value ); }
	private:
		::Decimal _value;
	};
}