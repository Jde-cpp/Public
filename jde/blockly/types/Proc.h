#pragma once
#include <cstdint>
#include <cmath>
#include <compare>
#include <jde/Exception.h>
#include <jde/Log.h>
#include <jde/markets/TypeDefs.h>
#include <jde/markets/types/MyOrder.h>
#include "../Exports-Executor.h"

//#ifdef TESTING
//	extern Jde::TimePoint g_now;
//ΓBE Jde::TimePoint& TestingNow();
//#endif

struct Contract;
namespace Jde::Markets{ struct Contract; struct TwsClient; class EventManagerTests; class OptionTests; }
namespace Jde::Markets::Proto::Results{ class ContractHours;}

template<> struct fmt::formatter<Jde::Decimal>
{
	constexpr α parse( fmt::format_parse_context& ctx )->decltype(ctx.begin()){ return ctx.end(); }
	ⓣ format( Jde::Decimal x, T& ctx )->decltype(ctx.out()){ return format_to( ctx.out(), "{:.2f}", (double)x ); }
};

namespace Jde::Markets::MBlockly
{
	namespace Blocks{ struct OptionTest; }
	struct Blockly;
	typedef std::chrono::system_clock::time_point TimePoint;
	typedef std::chrono::system_clock::duration Duration;
	struct ProcOrder; struct Amount; struct Size; struct LimitPriceException; struct BTick; struct Awaitable;
	typedef std::shared_ptr<std::vector<Proto::Results::ContractHours>> ContractHoursPtr;
	struct ΓBE Price /*notfinal*/
	{
		constexpr Price()ι=default;
		explicit Price( Decimal v )ι;//TODO move protected

		explicit operator bool()Ι{ return _value.has_value(); }
		explicit operator double()Ι
		{
			auto y = _value.has_value() ? (double)*_value : NAN;
			return y;
		}
		//friend auto operator<=>( const Price&, const Price& )ι = default;
		friend auto operator<( const Price& a, const Price& b)ι->bool{ return a && b && *a._value<*b._value; }
		friend auto operator>( const Price& a, const Price& b)ι->bool{ return !(a<b); }
		friend auto operator<=( const Price& a, const Price& b)ι->bool{ return a < b || a==b; }
		friend auto operator>=( const Price& a, const Price& b)ι->bool{ return a > b || a==b; }
		friend auto operator==( const Price& a, const Price& b)ι->bool{ return a && b && *a._value==*b._value; }
#define check CHECK( a ); CHECK( b );
		friend α operator-( const Price& a, const Price& b )ε->Price{ check return Price{ *a._value-*b._value }; }
		friend α operator+( const Price& a, const Price& b )ε->Price{ check return Price{ *a._value+*b._value }; }
		friend α operator/( const Price& a, const Price& b )ε->double{ check return (double)(*a._value / *b._value); }
		α ToString()Ι->string{ return (bool)*this ? fmt::format( "{}", *_value ) : "null"; }
	protected:

	private:
		operator Decimal(){ CHECK(_value); return *_value; }
		optional<Decimal> _value;
		friend Amount operator*( Price a, Size b )ε;
		friend ProcOrder; friend Amount; friend LimitPriceException; friend BTick; friend Blockly; friend OptionTests; friend Blocks::OptionTest;
	};

	struct Size final
	{
		Size()=default;
		//friend auto operator<=>( const Size&, const Size& )ι = default;
		α ToString()Ι->string{ return *this ? fmt::format("{:.0f}", (double)*_value) : "null"; }
		operator bool()Ι{ return _value.has_value(); }
	private:
		Size( Decimal value )ι:_value{ value }{}
		Size( double value )ι:_value{value}{}
		Size( long long value )ι:_value{(double)value}{}
		optional<Decimal> _value;
		constexpr static double Unitialized = NAN;
		friend Amount operator*( Price a, Size b )ε;
		friend ProcOrder; friend Amount; friend BTick; friend OptionTests;
	};
	struct LimitAmountException;
	struct Amount final
	{
		Amount()=default;
		friend Amount operator*( const Amount& a, double b )ε{ CHECK(a); return Amount( Decimal(ToDouble(a)*b) ) ;}
		//friend auto operator<=>( const Amount&, const Amount& )ι = default;
		α ToString()Ι->string{return *this ? fmt::format("{:.2f}", (double)*_value) : "null";}
		operator bool()Ι{ return _value.has_value(); }
	private:
		Amount( Decimal value )ι:_value{value}{}
		optional<Decimal> _value;
		constexpr static double Unitialized = NAN;
		friend ProcOrder; friend LimitAmountException;
		friend Amount operator*( Price a, Size b )ε;
		friend Blockly;
	};

	inline Amount operator*( Price a, Size b )ε{ check return Amount{*a._value * *b._value}; }


	struct ProcTimePoint;
	struct PositiveDuration;
	struct ProcDuration /*notfinal*/: protected Duration
	{
		typedef Duration base;
		explicit ProcDuration( Duration x )ι:base{x}{}
		friend ProcDuration operator/( const ProcDuration& a, double b )ε{ if( !llround(b) ) throw Exception{ SRCE_CUR, ELogLevel::Error, "'{}'divide by zero.", b }; base x = static_cast<const base&>(a)/llround(b); return ProcDuration{x}; }
	private:
		//operator Duration()Ι{ return *this; }
		friend ProcTimePoint; friend PositiveDuration; friend Blocks::OptionTest;
		friend ProcTimePoint operator+( ProcTimePoint a, ProcDuration b )ι;
	};

	struct ΓBE ProcTimePoint final /*: private TimePoint MSVC=LNK2005 */
	{
		constexpr ProcTimePoint()ι=default;
		static ProcTimePoint ClosingTime( ContractHoursPtr pHours )ε;

		explicit operator bool()Ι{ return base!=TimePoint{}; }//non-explicit screws with < operator in msvc
//		auto operator<=>( const ProcTimePoint& )const = default c2487 in msvc;
		friend bool operator==( const ProcTimePoint& a, const ProcTimePoint& b )ι{ return a.base==b.base; }
		friend bool operator>( const ProcTimePoint& a, const ProcTimePoint& b )ι{ return a.base>b.base; }
		friend bool operator>=( const ProcTimePoint& a, const ProcTimePoint& b )ι
		{
			//DBG( "{}>{}={}"sv, a.ToString(), b.ToString(), a.base>=b.base );
			return a.base>=b.base;
		}
		friend bool operator<( const ProcTimePoint& a, const ProcTimePoint& b )ι{ return a.base<b.base; }
		friend bool operator<=( const ProcTimePoint& a, const ProcTimePoint& b )ι{ return a.base<=b.base; }
		string ToString()Ι;
		static ProcTimePoint now()ι
		{
//#ifdef TESTING
			//if( g_now!=TimePoint::min() )
			//	return ProcTimePoint{g_now};
//#endif
			return ProcTimePoint{std::chrono::system_clock::now()};
		}

//#ifdef TESTING
//		TimePoint TP()Ι{ return *this; }
//#endif
		//friend auto operator<=>( const ProcTimePoint&, const ProcTimePoint& )ι = default;
	private:
		constexpr ProcTimePoint( TimePoint t )ι:base{t}{}
		constexpr static ProcTimePoint Unitialized()ι{ return ProcTimePoint{TimePoint{}}; }
		friend ProcDuration operator-( ProcTimePoint me, ProcTimePoint b )ι{ return ProcDuration{me.base-b.base}; }
		friend ProcTimePoint operator+( ProcTimePoint a, ProcDuration b )ι
		{
			//Duration bTemp = (Duration)b;
			return ProcTimePoint{a.base+(Duration)b};
		}
		TimePoint base;
		friend Awaitable;
		friend Jde::Markets::EventManagerTests; friend Blocks::OptionTest; friend OptionTests; friend ProcOrder;
	};

	struct PositiveDuration final : protected ProcDuration
	{
		PositiveDuration( ProcDuration x )ε;

		friend ProcDuration operator/( PositiveDuration a, int_fast64_t b )ι{ return ProcDuration{ ((Duration)a)/b }; }
		static PositiveDuration LiquidTimeLeft( const Contract& contract, ProcTimePoint reference )ε;
		static PositiveDuration TradingTimeLeft( const Contract& contract, ProcTimePoint reference )ε;
	};

	struct PriceConst : Price
	{
		/*consteval clang crashconstexpr*/ PriceConst( double x )ε:Price{x}{};
	};

	struct ΓBE Limits
	{
		α Price()Ι->MBlockly::Price{return _price;}
		α SetPrice( MBlockly::Price price )ι->void;
		α Value()Ι->MBlockly::Amount{return _value;}
		α SetValue( MBlockly::Amount value )ι->void;
	private:
		MBlockly::Price _price;
		Amount _value;
	};

	struct Account
	{
		Amount Available;
	};
/**************************************************************/
	struct ProcOrder final: private MyOrder
	{
		ProcOrder()ι:ProcOrder{0}{};
		ProcOrder( long orderId )ι:MyOrder{::Order{orderId}}{};
		ProcOrder( const MyOrder& x )ι:MyOrder{x}{}
		operator bool()Ι{ return MyOrder::action.size(); }
		α AccountNumber()Ι->str{ return MyOrder::account; }
		α OrderId()Ι{ return MyOrder::orderId; }
		α IsBuy()Ι{ return MyOrder::IsBuy(); }
		α Limit()Ι{ return Price{ ToDecimal(MyOrder::lmtPrice) }; }
		α PostToAts(){ return MyOrder::postToAts; }//TODO remove
		α SetLimit( Price limit )ε{ CHECK(limit); MyOrder::lmtPrice = (double)limit; }
		α Quantity()Ι{ return Size{Decimal{MyOrder::totalQuantity}}; }
		α LastUpdate()Ι{ return ProcTimePoint{MyOrder::LastUpdate}; }
		ΓBE α SetLastUpdate( ProcTimePoint x )ι->void;
		α Bump( Price price )ε{ CHECK(price); MyOrder::lmtPrice+=(double)price; }
		α Place( sp<::Contract> p )ε->void;
		α VolatilityType()Ι{ return volatilityType; }
		α ToProto()Ι->up<Proto::Order>{ return MyOrder::ToProto(); }
	private:
		α Base()Ι->const MyOrder&{ return *this;}
		friend Awaitable;
	};
}
#undef check