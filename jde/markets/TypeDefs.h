#pragma once

#define DECIMAL_GLOBAL_ROUNDING 0
#define DECIMAL_CALL_BY_REFERENCE 0
#pragma warning( push )
#pragma warning( disable : 4324)
#include "C:\Users\duffyj\source\repos\libraries\IntelRDFPMathLib20U2\LIBRARY\src\bid_conf.h"
#include "C:\Users\duffyj\source\repos\libraries\IntelRDFPMathLib20U2\LIBRARY\src\bid_functions.h"
#pragma warning( pop )
namespace Jde::Markets
{
	using Decimal=unsigned long long;
	typedef int32 AccountPK;
	typedef long ContractPK;
	typedef int32 DecisionTreePK;
	typedef uint8 MetricPK;

	typedef long ReqId;//
	typedef unsigned long long OrderId;
	typedef uint16 MinuteIndex;//TODO change to int

	typedef double Amount; //Decimal2
	typedef double PositionAmount;
	Ξ ToPosition( double value )noexcept->PositionAmount{ return value; }
	Ξ ToDouble( Decimal v )noexcept->double{ unsigned int _; double y = bid64_to_binary64( v, 0, &_ ); return y; }
	Ξ ToDecimal( double v )noexcept->Decimal{ unsigned int _; Decimal y = binary64_to_bid64( v, 0, &_ ); return y; }
}
