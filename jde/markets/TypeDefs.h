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
#include "Decimal.h"

namespace Jde::Markets
{
	using AccountPK=uint32;
	using ContractPK=long;
	using DecisionTreePK=int32;
	using MetricPK=uint8;

	using ReqId=long;
	using OrderId=unsigned long long;
	using MinuteIndex=uint16;//TODO change to int

	using Amount=double;
	using PositionAmount=double;
}
