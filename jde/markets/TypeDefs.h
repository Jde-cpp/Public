#pragma once
#ifndef JDE_MARKETS_TYPE_DEFS
#define JDE_MARKETS_TYPE_DEFS

namespace Jde::Markets
{
	typedef int32 AccountPK;
	typedef long ContractPK;
	typedef int32 DecisionTreePK;
	typedef uint8 MetricPK;

	typedef long ReqId;//
	typedef unsigned long long OrderId;
	typedef uint16 MinuteIndex;//TODO change to int

	typedef double Amount; //Decimal2
	typedef double PositionAmount;
	inline PositionAmount ToPosition( double value ){ return value; }
}
#endif