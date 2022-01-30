#pragma once

#define DECIMAL_GLOBAL_ROUNDING 0
#define DECIMAL_CALL_BY_REFERENCE 0
#pragma warning( push )
#pragma warning( disable : 4324)
#ifndef _WCHAR_T_DEFINED
	#define _WCHAR_T_DEFINED
#endif
#include <bid_conf.h>
#include <bid_functions.h>
#pragma warning( pop )
#include "../TypeDefs.h"
namespace Jde::Markets
{
	using Decimal=unsigned long long;
	//typedef int32 AccountPK;
	using AccountPK=uint32;
	using ContractPK=long;
	using DecisionTreePK=int32;
	using MetricPK=uint8;

	using ReqId=long;
	using OrderId=unsigned long long;
	using MinuteIndex=uint16;//TODO change to int

	using Amount=double;
	using PositionAmount=double;
	Ξ ToPosition( double value )noexcept->PositionAmount{ return value; }
	Ξ ToDouble( Decimal v )noexcept->double{ unsigned int _; double y = bid64_to_binary64( v, 0, &_ ); return y; }
	Ξ ToDecimal( double v )noexcept->Decimal{ unsigned int _; Decimal y = binary64_to_bid64( v, 0, &_ ); return y; }
	Ξ ToString( Decimal v )noexcept->string{ char y[64]; unsigned int _; bid64_to_string(y, v, &_ ); return y; }
	Ξ Subtract( Decimal x1, Decimal x2)noexcept->Decimal{ unsigned int _; return bid64_sub(x1, x2, 0, &_); }
	Ξ Add( Decimal x1, Decimal x2)noexcept->Decimal{ unsigned int _; return bid64_add(x1, x2, 0, &_); }
}