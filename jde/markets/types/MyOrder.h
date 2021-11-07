#pragma once
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

	struct JDE_MARKETS_EXPORT MyOrder : public ::Order
	{
		MyOrder()noexcept=default;
		MyOrder( ::OrderId orderId, const Proto::Order& order )noexcept;
		MyOrder( const ::Order& order )noexcept:Order{order}{}

		bool IsBuy()const noexcept{ return action=="BUY"; } void IsBuy( bool value )noexcept{ action = value ? "BUY" : "SELL"; }
		Proto::ETimeInForce TimeInForce()const noexcept; void TimeInForce( Proto::ETimeInForce value )noexcept;
		Proto::EOrderType OrderType()const noexcept; void OrderType( Proto::EOrderType value )noexcept;
		std::unique_ptr<Proto::Order> ToProto()const noexcept;
		static time_t ParseDateTime( const std::string& date )noexcept;
		static std::string ToDateString( time_t date )noexcept;
		static Proto::Results::OrderState* ToAllocatedProto( const ::OrderState& state )noexcept;

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
		mutable std::chrono::system_clock::time_point LastUpdate;
	};
	inline MyOrder::Fields operator|(MyOrder::Fields a, MyOrder::Fields b){ return (MyOrder::Fields)( (uint)a | (uint)b ); }
	inline MyOrder::Fields operator|=(MyOrder::Fields& a, MyOrder::Fields b){ return a = (MyOrder::Fields)( (uint)a | (uint)b ); }

	struct OrderStatus final
	{
		::OrderId Id;
		EOrderStatus Status{EOrderStatus::None};
		double Filled;
		double Remaining;
		double AverageFillPrice;
		int_fast32_t PermId;
		int_fast32_t ParentId;
		double LastFillPrice;
		std::string WhyHeld;
		double MarketCapPrice;

		enum class Fields : uint_fast8_t
		{
			None = 0,
			Status      = 1 << 1,
			Filled      = 1 << 2,
			Remaining   = 1 << 3,
			All = std::numeric_limits<uint_fast8_t>::max()
		};
		Fields Changes( const OrderStatus& status, Fields fields )const noexcept;
	};
	inline OrderStatus::Fields operator|(OrderStatus::Fields a, OrderStatus::Fields b){ return (OrderStatus::Fields)( (uint_fast8_t)a | (uint_fast8_t)b ); }
	inline OrderStatus::Fields operator|=(OrderStatus::Fields& a, OrderStatus::Fields b){ return a = (OrderStatus::Fields)( (uint)a | (uint)b ); }

	struct OrderState : ::OrderState
	{
		OrderState():
			::OrderState{}
		{}
		OrderState( const ::OrderState& base ):
			::OrderState{base}
		{}

		enum class Fields : uint_fast8_t
		{
			None            = 0,
			Status          = 1 << 1,
			CompletedTime   = 1 << 2,
			CompletedStatus = 1 << 3,
			All = std::numeric_limits<uint_fast8_t>::max()
		};
		Fields Changes( const OrderState& state, Fields fields )const noexcept;
	};
	inline OrderState::Fields operator| (OrderState::Fields a, OrderState::Fields b){ return (OrderState::Fields)( (uint_fast8_t)a | (uint_fast8_t)b ); }
	inline OrderState::Fields operator|=(OrderState::Fields& a, OrderState::Fields b){ return a = (OrderState::Fields)( (uint)a | (uint)b ); }
}