﻿#pragma once
#ifdef _MSC_VER
#pragma push_macro("assert")
#undef assert
#pragma warning( disable : 4267 )
#include <Order.h>
#include <OrderState.h>
#include <CommonDefs.h>
#pragma warning( default : 4267 )
#pragma pop_macro("assert")
#endif
#include "../Exports.h"
#pragma warning( disable : 4244 )
#pragma warning( disable : 4996 )
#include "proto/results.pb.h"
#pragma warning( default : 4244 )
#pragma warning( default : 4996 )


namespace Jde::Markets::Proto{ class Order; enum ETimeInForce : int; enum EOrderType : int; }
namespace Jde::Markets::Proto::Results{ enum EOrderStatus : int; }
namespace Jde::Markets
{
	using Proto::Results::EOrderStatus;
	namespace Proto
	{
		namespace Results{ class OrderState; }
		namespace IB{ enum ETimeInForce:int; class Order; }
	}

	struct ΓM MyOrder : public ::Order
	{
		MyOrder()noexcept=default;
		MyOrder( ::OrderId orderId, const Proto::Order& order )noexcept;
		MyOrder( const ::Order& order )noexcept:Order{order}{}

		α IsBuy()const noexcept->bool{ return action=="BUY"; } void IsBuy( bool value )noexcept{ action = value ? "BUY" : "SELL"; }
		α TimeInForce()const noexcept->Proto::ETimeInForce; void TimeInForce( Proto::ETimeInForce value )noexcept;
		α OrderType()const noexcept->Proto::EOrderType; void OrderType( Proto::EOrderType value )noexcept;
		α ToProto()const noexcept->up<Proto::Order>;
		Ω ParseDateTime( str date )noexcept->time_t;
		Ω ToDateString( time_t date )noexcept->string;

		enum class Fields : uint
		{
			None        = 0,
			LastUpdate 	= 1 << 1,
			Limit 		= 1 << 2,
			Quantity    = 1 << 3,
			Action 		= 1 << 4,
			Type	 		= 1 << 5,
			Aux	 		= 1 << 6,
			Transmit		= 1 << 7,
			Account		= 1 << 8,
			All = std::numeric_limits<uint>::max() //(uint)~0
		};
		Fields Changes( const MyOrder& status, Fields fields )const noexcept;
		mutable TP LastUpdate;
	};
	Ξ operator|(MyOrder::Fields a, MyOrder::Fields b)noexcept->MyOrder::Fields{ return (MyOrder::Fields)( (uint)a | (uint)b ); }
	Ξ operator|=(MyOrder::Fields& a, MyOrder::Fields b)noexcept->MyOrder::Fields{ return a = (MyOrder::Fields)( (uint)a | (uint)b ); }

	struct OrderStatus final
	{
		::OrderId Id;
		EOrderStatus Status{EOrderStatus::None};
		α ToProto()const noexcept->up<Proto::Results::OrderStatus>;
		double Filled;
		double Remaining;
		double AverageFillPrice;
		int_fast32_t PermId;
		int_fast32_t ParentId;
		double LastFillPrice;
		string WhyHeld;
		double MarketCapPrice;

		enum class Fields : uint_fast8_t
		{
			None = 0,
			Status      = 1 << 1,
			Filled      = 1 << 2,
			Remaining   = 1 << 3,
			All = std::numeric_limits<uint_fast8_t>::max()
		};
		Fields Changes( const OrderStatus& status )const noexcept;
	};
	Ξ operator|(OrderStatus::Fields a, OrderStatus::Fields b)->OrderStatus::Fields{ return (OrderStatus::Fields)( (uint_fast8_t)a | (uint_fast8_t)b ); }
	Ξ operator|=(OrderStatus::Fields& a, OrderStatus::Fields b)->OrderStatus::Fields{ return a = (OrderStatus::Fields)( (uint)a | (uint)b ); }

	α ToProto( const ::OrderState& state )noexcept->up<Proto::Results::OrderState>;
/*	struct OrderState final: ::OrderState
	{
		OrderState():
			::OrderState{}
		{}
		OrderState( const ::OrderState& base ):
			::OrderState{base}
		{}
*/
	enum class OrderStateFields : uint_fast8_t
	{
		None            = 0,
		Status          = 1 << 1,
		CompletedTime   = 1 << 2,
		CompletedStatus = 1 << 3,
		All = std::numeric_limits<uint_fast8_t>::max()
	};
	α OrderStateChanges( const ::OrderState& a, const ::OrderState& b )noexcept->OrderStateFields;
//	};
	Ξ operator| (OrderStateFields a, OrderStateFields b)->OrderStateFields{ return (OrderStateFields)( (uint_fast8_t)a | (uint_fast8_t)b ); }
	Ξ operator|=(OrderStateFields& a, OrderStateFields b)->OrderStateFields{ return a = (OrderStateFields)( (uint)a | (uint)b ); }
}