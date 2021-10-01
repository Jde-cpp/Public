#pragma once
#ifndef JDE_TICK
#define JDE_TICK

#include <variant>
#include <map>
#include <bitset>
#include <CommonDefs.h>
#include "../Exports.h"

namespace Jde{ template<typename> class Vector; }
namespace Jde::Markets
{
	class OptionTests;
	typedef long ContractPK;
	template<class T> using sp = std::shared_ptr<T>;
	template<class T> using up = std::unique_ptr<T>;

	namespace Proto
	{
		namespace Requests{ enum ETickList:int; }
		namespace Results{ class OptionCalculation; class TickNews; enum ETickType:int; class MessageUnion; }
	}
	using Proto::Requests::ETickList; using Proto::Results::ETickType;
	struct OptionComputation
	{
		std::unique_ptr<Proto::Results::OptionCalculation> ToProto( ContractPK contractId, ETickType tickType )const noexcept;
		bool operator==(const OptionComputation& x)const noexcept{ return memcmp(this, &x, sizeof(OptionComputation))==0; }
		bool ReturnBased;//vs price based TickAttrib;
		double ImpliedVol;
		double Delta;
		double OptPrice;
		double PVDividend;
		double Gamma;
		double Vega;
		double Theta;
		double UndPrice;
	};

	struct News
	{
		std::unique_ptr<Proto::Results::TickNews> ToProto( ContractPK contractId )const noexcept;
		const time_t TimeStamp;
		const string ProviderCode;
		const string ArticleId;
		const string Headline;
		const string ExtraData;
	};

	struct JDE_MARKETS_EXPORT Tick
	{
		typedef std::bitset<91> Fields;//ETickType::NOT_SET+1
		typedef std::variant<nullptr_t,uint,int,double,time_t,string,OptionComputation,sp<Vector<News>>> TVariant;
		Tick()=default;//TODO try to remove
		Tick( ContractPK id ):ContractId{id}{}
		Tick( ContractPK id, TickerId tickId ):ContractId{id},TwsRequestId{tickId}{};

		bool SetString( ETickType type, str value )noexcept;
		bool SetInt( ETickType type, _int value )noexcept;
		bool SetPrice( ETickType type, double value/*, const ::TickAttrib& attribs*/ )noexcept;
		void SetPrices( double bidSize, double bid, double askSize, double ask )noexcept;
		bool SetDouble( ETickType type, double value )noexcept;
		bool SetOptionComputation( ETickType type, OptionComputation&& v )noexcept;
		bool FieldEqual( const Tick& other, ETickType tick )const noexcept;
		bool IsSet( ETickType type )const noexcept{ return _setFields[type]; }
		bool HasRatios()const noexcept;
		void AddNews( News&& news )noexcept;
		Fields SetFields()const noexcept{ return _setFields; }
		std::map<string,double> Ratios()const noexcept;
		Proto::Results::MessageUnion ToProto( ETickType type )const noexcept;
		void AddProto( ETickType type, std::vector<Proto::Results::MessageUnion>& messages )const noexcept;

		static Fields PriceFields()noexcept;
		ContractPK ContractId{0};
		TickerId TwsRequestId{0};
		long long BidSize;
		double Bid;
		double Ask;
		long long AskSize;
		double LastPrice;
		double LastSize;
		double High;
		double Low;
		uint Volume;
		double ClosePrice;
		OptionComputation BID_OPTION_COMPUTATION;
		OptionComputation ASK_OPTION_COMPUTATION;
		OptionComputation LAST_OPTION_COMPUTATION;
		OptionComputation MODEL_OPTION;
		double OpenTick;
		double Low13Week;
		double High13Week;
		double Low26Week;
		double High26Week;
		double Low52Week;
		double High52Week;
		uint AverageVolume;
		uint OPEN_INTEREST;
		double OptionHistoricalVol;
		double OptionImpliedVol;
		double OPTION_BID_EXCH;
		double OPTION_ASK_EXCH;
		uint OPTION_CALL_OPEN_INTEREST;
		uint OPTION_PUT_OPEN_INTEREST;
		uint OPTION_CALL_VOLUME;
		uint OPTION_PUT_VOLUME;
		double INDEX_FUTURE_PREMIUM;
		string BidExchange;
		string AskExchange;
		uint AUCTION_VOLUME;
		double AUCTION_PRICE;
		double AUCTION_IMBALANCE;
		double MarkPrice;
		double BID_EFP_COMPUTATION;
		double ASK_EFP_COMPUTATION;
		double LAST_EFP_COMPUTATION;
		double OPEN_EFP_COMPUTATION;
		double HIGH_EFP_COMPUTATION;
		double LOW_EFP_COMPUTATION;
		double CLOSE_EFP_COMPUTATION;
		time_t LastTimestamp;
		double SHORTABLE;
		string RatioString;
		string RT_VOLUME;
		double Halted;
		double BID_YIELD;
		double ASK_YIELD;
		double LAST_YIELD;
		double CUST_OPTION_COMPUTATION;
		uint TRADE_COUNT;
		double TRADE_RATE;
		uint VOLUME_RATE;
		double LAST_RTH_TRADE;
		double RT_HISTORICAL_VOL;
		string DividendString;
		double BOND_FACTOR_MULTIPLIER;
		double REGULATORY_IMBALANCE;
		sp<Vector<News>> NewsPtr;
		uint SHORT_TERM_VOLUME_3_MIN;
		uint SHORT_TERM_VOLUME_5_MIN;
		uint SHORT_TERM_VOLUME_10_MIN;
		double DELAYED_BID;
		double DELAYED_ASK;
		double DELAYED_LAST;
		uint DELAYED_BID_SIZE;
		uint DELAYED_ASK_SIZE;
		uint DELAYED_LAST_SIZE;
		double DELAYED_HIGH;
		double DELAYED_LOW;
		uint DELAYED_VOLUME;
		double DELAYED_CLOSE;
		double DELAYED_OPEN;
		uint RT_TRD_VOLUME;
		double CREDITMAN_MARK_PRICE;
		double CREDITMAN_SLOW_MARK_PRICE;
		double DELAYED_BID_OPTION_COMPUTATION;
		double DELAYED_ASK_OPTION_COMPUTATION;
		double DELAYED_LAST_OPTION_COMPUTATION;
		double DELAYED_MODEL_OPTION_COMPUTATION;
		string LastExchange;
		time_t LAST_REG_TIME;
		uint FUTURES_OPEN_INTEREST;
		uint AVG_OPT_VOLUME;
		time_t DELAYED_LAST_TIMESTAMP;
		uint SHORTABLE_SHARES;
		int NOT_SET;
	private:
		Fields _setFields;
		TVariant Variant( ETickType type )const noexcept;
		friend OptionTests;
	};
}
#endif