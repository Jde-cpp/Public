#pragma once
#include <cstdint>
#include <cmath>
#include <compare>
#ifdef _MSC_VER
//	#include <fmt/core.h>
#endif
#include <jde/Exception.h>
#include <jde/Log.h>
#include <jde/markets/types/MyOrder.h>
#include "../Exports-Executor.h"

//#ifdef TESTING
//	extern Jde::TimePoint g_now;
//JDE_BLOCKLY_EXECUTOR Jde::TimePoint& TestingNow();
//#endif

struct Contract;
namespace Jde::Markets{ struct Contract; struct TwsClient; class EventManagerTests; class OptionTests; }
namespace Jde::Markets::Proto::Results{ class ContractHours;}
namespace Jde::Markets::MBlockly
{
	using std::string;
	namespace Blocks{ struct OptionTest; }
	struct Blockly;
	typedef std::chrono::system_clock::time_point TimePoint;
	typedef std::chrono::system_clock::duration Duration;
	struct ProcOrder; struct Amount; struct Size; struct LimitPriceException; struct BTick; struct Awaitable;
	typedef std::shared_ptr<std::vector<Proto::Results::ContractHours>> ContractHoursPtr;
	struct JDE_BLOCKLY_EXECUTOR Price /*notfinal*/
	{
		constexpr Price()noexcept=default;
		explicit Price( double v )noexcept(false);//TODO move protected


		bool empty()const noexcept{ return _value==Unitialized; }
		friend auto operator<=>( const Price&, const Price& )noexcept = default;
		friend Price operator-( const Price& me, const Price& other )noexcept{ return Price( me._value-other._value ); }
		friend Price operator+( const Price& me, const Price& other )noexcept{ return Price( me._value+other._value ); }
		friend double operator/( const Price& me, const Price& other )noexcept{ return me._value/other._value; }
		std::string ToString()const noexcept{ return fmt::format("{:.2f}", _value); }
	protected:

	private:
		double _value{Unitialized};
		constexpr static double Unitialized = NAN;
		friend Amount operator*( Price a, Size b )noexcept;
		friend ProcOrder; friend Amount; friend LimitPriceException; friend BTick; friend Blockly; friend OptionTests; friend Blocks::OptionTest;
	};

	struct Size final
	{
		Size()=default;
		friend auto operator<=>( const Size&, const Size& )noexcept = default;
		string ToString()const{ return fmt::format("{:.0f}", _value); }
	private:
		Size( double value )noexcept:_value{value}{}
		Size( long long value )noexcept:_value{(double)value}{}
		double _value{Unitialized};
		constexpr static double Unitialized = NAN;
		friend Amount operator*( Price a, Size b )noexcept;
		friend ProcOrder; friend Amount; friend BTick; friend OptionTests;
	};
	struct LimitAmountException;
	struct Amount final
	{
		Amount()=default;
		friend Amount operator*( const Amount& me, double other )noexcept{ return Amount( me._value*other ) ;}
		friend auto operator<=>( const Amount&, const Amount& )noexcept = default;
		string ToString()const noexcept{return fmt::format("{:.2f}", _value);}
	private:
		Amount( double value )noexcept:_value{value}{}
		double _value{Unitialized};
		constexpr static double Unitialized = NAN;
		friend ProcOrder; friend LimitAmountException;
		friend Amount operator*( Price a, Size b )noexcept;
		friend Blockly;
	};
	//Amount operator*( Price a, Size b )noexcept{ return Amount{};}

	inline Amount operator*( Price a, Size b )noexcept{ return Amount{a._value*b._value}; }


	struct ProcTimePoint;
	struct PositiveDuration;
	struct ProcDuration /*notfinal*/: protected Duration
	{
		typedef Duration base;
		explicit ProcDuration( Duration x )noexcept:base{x}{}
		friend ProcDuration operator/( const ProcDuration& a, double b )noexcept(false){ THROW_IF(!llround(b), "divide by zero."); base x = static_cast<const base&>(a)/llround(b); return ProcDuration{x}; }
	private:
		//operator Duration()const noexcept{ return *this; }
		friend ProcTimePoint; friend PositiveDuration; friend Blocks::OptionTest;
		friend ProcTimePoint operator+( ProcTimePoint a, ProcDuration b )noexcept;
	};

	struct JDE_BLOCKLY_EXECUTOR ProcTimePoint final /*: private TimePoint MSVC=LNK2005 */
	{
		constexpr ProcTimePoint()noexcept=default;
		static ProcTimePoint ClosingTime( ContractHoursPtr pHours )noexcept(false);

		explicit operator bool()const noexcept{ return base!=TimePoint{}; }//non-explicit screws with < operator in msvc
//		auto operator<=>( const ProcTimePoint& )const = default c2487 in msvc;
		friend bool operator==( const ProcTimePoint& a, const ProcTimePoint& b )noexcept{ return a.base==b.base; }
		friend bool operator>( const ProcTimePoint& a, const ProcTimePoint& b )noexcept{ return a.base>b.base; }
		friend bool operator>=( const ProcTimePoint& a, const ProcTimePoint& b )noexcept
		{
			//DBG( "{}>{}={}"sv, a.ToString(), b.ToString(), a.base>=b.base );
			return a.base>=b.base;
		}
		friend bool operator<( const ProcTimePoint& a, const ProcTimePoint& b )noexcept{ return a.base<b.base; }
		friend bool operator<=( const ProcTimePoint& a, const ProcTimePoint& b )noexcept{ return a.base<=b.base; }
		string ToString()const noexcept;
		static ProcTimePoint now()noexcept
		{
//#ifdef TESTING
			//if( g_now!=TimePoint::min() )
			//	return ProcTimePoint{g_now};
//#endif
			return ProcTimePoint{std::chrono::system_clock::now()};
		}

//#ifdef TESTING
//		TimePoint TP()const noexcept{ return *this; }
//#endif
		//friend auto operator<=>( const ProcTimePoint&, const ProcTimePoint& )noexcept = default;
	private:
		constexpr ProcTimePoint( TimePoint t )noexcept:base{t}{}
		constexpr static ProcTimePoint Unitialized()noexcept{ return ProcTimePoint{TimePoint{}}; }
		friend ProcDuration operator-( ProcTimePoint me, ProcTimePoint other )noexcept{ return ProcDuration{me.base-other.base}; }
		friend ProcTimePoint operator+( ProcTimePoint a, ProcDuration b )noexcept
		{
			//Duration bTemp = (Duration)b;
			return ProcTimePoint{a.base+(Duration)b};
		}
		TimePoint base;
		friend Awaitable;
		friend Jde::Markets::EventManagerTests; friend Blocks::OptionTest; friend OptionTests;
	};

	struct PositiveDuration final : protected ProcDuration
	{
		PositiveDuration( ProcDuration x )noexcept(false);

		friend ProcDuration operator/( PositiveDuration a, int_fast64_t b )noexcept{ return ProcDuration{ ((Duration)a)/b }; }
		static PositiveDuration LiquidTimeLeft( const Contract& contract, ProcTimePoint reference )noexcept(false);
		static PositiveDuration TradingTimeLeft( const Contract& contract, ProcTimePoint reference )noexcept(false);
	};

	struct PriceConst : Price
	{
		/*consteval clang crashconstexpr*/ PriceConst( double x )noexcept(false):Price{x}{};
	};

	struct Limits
	{
		MBlockly::Price Price;
		Amount Value;
	};

	struct Account
	{
		Amount Available;
	};
/**************************************************************/
	struct ProcOrder final: private MyOrder
	{
		ProcOrder()noexcept:ProcOrder{0}{};
		ProcOrder( long orderId )noexcept:MyOrder{::Order{orderId}}{};
		ProcOrder( const MyOrder& x )noexcept:MyOrder{x}{}
		//ProcOrder( const ProcOrder& x )noexcept:MyOrder{x}, _lastUpdate{x._lastUpdate}{}

		const std::string& AccountNumber()const noexcept{ return MyOrder::account; }
		const int32 OrderId()const noexcept{ return MyOrder::orderId; }

		Price Limit()const noexcept{ return Price{MyOrder::lmtPrice }; }
		void SetLimit( Price limit )noexcept{ MyOrder::lmtPrice = limit._value; }
		Size Quantity()const noexcept{ return Size{MyOrder::totalQuantity }; }
		ProcTimePoint LastUpdate()const noexcept{ return _lastUpdate; } void SetLastUpdate( ProcTimePoint x )noexcept
		{
			_lastUpdate=x;
			DBG( "Order.LastUpdate='{}'."sv, _lastUpdate.ToString() );
		}
		void Bump( Price price )noexcept{ MyOrder::lmtPrice+=price._value; }
		void Place( TwsClient& tws, const ::Contract& contract )noexcept;
		int VolatilityType()const noexcept{ return volatilityType; }
	private:
		const MyOrder& Base()const noexcept{ return *this;}
		ProcTimePoint _lastUpdate;
		friend Awaitable;
	};
/************************************************************
	struct LimitPriceException final: Exception
	{
		LimitPriceException( Price current, Price limit )noexcept:
			Exception( "'{}' is over the price limit '{}'.", current._value, limit._value )
		{}
	};
	struct LimitAmountException final: Exception
	{
		LimitAmountException( Amount current, Amount limit )noexcept:
			Exception( "'{}' is over the value limit '{}'.", current._value, limit._value )
		{}
	};*/
}