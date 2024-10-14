#pragma once

#include <variant>
#include <map>
#include <bitset>
#include <CommonDefs.h>
#include "../Exports.h"
#include "../TypeDefs.h"
#include <jde/markets/types/proto/ib.pb.h>

namespace Jde{ template<typename> class Vector; }
namespace Jde::Markets
{
	class OptionTests;
	typedef long ContractPK;
	namespace Proto
	{
		namespace Requests{ enum ETickList:int; }
		namespace Results{ class OptionCalculation; class TickNews; enum ETickType:int; class Message; }
	}
	using Proto::Requests::ETickList; using Proto::Results::ETickType;
	struct OptionComputation
	{
		up<Proto::Results::OptionCalculation> ToProto( ContractPK contractId, ETickType tickType )const noexcept;
		α operator==(const OptionComputation& x)const noexcept->bool{ return memcmp(this, &x, sizeof(OptionComputation))==0; }
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
		up<Proto::Results::TickNews> ToProto( ContractPK contractId )const noexcept;
		const time_t TimeStamp;
		const string ProviderCode;
		const string ArticleId;
		const string Headline;
		const string ExtraData;
	};

	struct ΓM Tick
	{
		using Fields=std::bitset<91> ;//ETickType::NOT_SET+1
		using TVariant=std::variant<nullptr_t,uint,int,double,time_t,string,OptionComputation,sp<Vector<News>>>;
		Tick()=default;//TODO try to remove
		Tick( ContractPK id ):ContractId{id}{}
		Tick( ContractPK id, TickerId tickId ):ContractId{id},TwsRequestId{tickId}{};

		α SetString( ETickType type, str value )noexcept->bool;
		α SetInt( ETickType type, _int value )noexcept->bool;
		α SetPrice( ETickType type, double value/*, const ::TickAttrib& attribs*/ )noexcept->bool;
		α SetPrices( Decimal bidSize, double bid, Decimal askSize, double ask )noexcept->void;
		α SetDecimal( ETickType type, Decimal value )noexcept->bool;
		α SetDouble( ETickType type, double value )noexcept->bool;
		α SetOptionComputation( ETickType type, OptionComputation&& v )noexcept->bool;
		α FieldEqual( const Tick& other, ETickType tick )const noexcept->bool;
		α IsSet( ETickType type )const noexcept->bool{ return _setFields[type]; }
		α HasRatios()const noexcept->bool;
		α AddNews( News&& news )noexcept->void;
		α SetFields()const noexcept->Fields{ return _setFields; }
		α Ratios()const noexcept->std::map<string,double>;//don't use boost because of blockly
		α ToProto( ETickType type )const noexcept->Proto::Results::Message;
		α ToProto()const noexcept->up<Jde::Markets::Proto::Tick>;
		α AddProto( ETickType type, std::vector<Proto::Results::Message>& messages )const noexcept->void;
		α AllSet( Markets::Tick::Fields fields )const noexcept->bool;
		static Fields PriceFields()noexcept;
		ContractPK ContractId{0};
		TickerId TwsRequestId{0};
		Decimal BidSize;
		double Bid;
		double Ask;
		Decimal AskSize;
		double LastPrice;
		Decimal LastSize;
		double High;
		double Low;
		Decimal Volume;
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
		Decimal AverageVolume;
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
		Decimal ShortableShares;
		int NOT_SET;
	private:
		Fields _setFields;
		TVariant Variant( ETickType type )const noexcept;
		friend OptionTests;
	};
}