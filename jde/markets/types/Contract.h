#pragma once
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/ib.pb.h>
#include <jde/markets/types/proto/results.pb.h>
#pragma warning( default : 4244 )

#include <jde/markets/TypeDefs.h>
#define Φ ΓM α
struct ContractDetails;
struct Contract;

namespace Jde::Markets
{
	α Liquid( const ::ContractDetails& c )noexcept->sp<vector<Proto::Results::ContractHours>>;
	α Rth( const ::ContractDetails& c )noexcept->sp<vector<Proto::Results::ContractHours>>;

	typedef std::chrono::system_clock::time_point TimePoint;
	namespace Proto::Results{ class ContractDetail; class ContractHours; }
	using Proto::Exchanges;

#pragma region DeltaNeutralContract
	struct DeltaNeutralContract
	{
		DeltaNeutralContract()noexcept{};
		DeltaNeutralContract( const Proto::DeltaNeutralContract& proto )noexcept;
		long Id{0};
		double Delta{0.0};
		double Price{0.0};

		sp<Proto::DeltaNeutralContract> ToProto( bool stupidPointer )const noexcept;
	};
#pragma endregion
#pragma region SecurityRight
	using SecurityRight = Proto::SecurityRight;
	Φ ToSecurityRight( sv name )noexcept->SecurityRight;
	Φ ToString( SecurityRight right )noexcept->sv;
#pragma endregion
#pragma region SecurityType
	using SecurityType=Proto::SecurityType;
	Φ ToSecurityType( sv inputName )noexcept->SecurityType;
	Φ ToString( SecurityType type )noexcept->sv;
#pragma endregion
#pragma region ComboLeg
	struct ComboLeg
	{
		ComboLeg( const Proto::ComboLeg& proto )noexcept;

		ContractPK ConId{0};
		long Ratio{0};
		std::string Action; //BUY/SELL/SSHORT
		std::string Exchange;
		long OpenClose{0}; // LegOpenClose enum values

		// for stock legs when doing short sale
		long ShortSaleSlot{0}; // 1 = clearing broker, 2 = third party
		std::string	DesignatedLocation;
		int_fast32_t ExemptCode{-1};

		α SetProto( Proto::ComboLeg* pProto )const noexcept->void;
		α operator==( const ComboLeg& other) const noexcept->bool
		{
			return ConId == other.ConId && Ratio == other.Ratio && OpenClose == other.OpenClose
				&& ShortSaleSlot == other.ShortSaleSlot && ExemptCode == other.ExemptCode && Action == other.Action
				&& Exchange == other.Exchange &&  DesignatedLocation == other.DesignatedLocation;
		}
	};
	typedef sp<ComboLeg> ComboLegPtr_;
#pragma endregion
#pragma region Contract
	struct ΓM Contract
	{
		static constexpr sv CacheDetailsFormat="ContractDetails.{}";
		Contract()=default;
		explicit Contract( ContractPK id, sv symbol="" )noexcept;
		Contract( ContractPK id, Proto::Currencies currency, sv localSymbol, uint_fast32_t multiplier, sv name, Exchanges exchange, sv symbol, sv tradingClass, TimePoint issueDate=TimePoint::max() )noexcept;
		Contract( const ::Contract& contract )noexcept;
		Contract( const Contract& contract )noexcept=default;
		Contract( const ContractDetails& details )noexcept;
		Contract( const Proto::Contract& contract )noexcept;
		~Contract();
		α operator <( const Contract &b )const noexcept->bool{return Id<b.Id;}

		string Display()const noexcept;
		sp<::Contract> ToTws()const noexcept;
		up<Proto::Contract> ToProto()const noexcept;
		ContractPK Id{0};
		string Symbol;
		SecurityType SecType{SecurityType::Stock};//"STK", "OPT"
		DayIndex Expiration{0};
		double Strike{0.0};
		SecurityRight Right{SecurityRight::None};
		uint_fast32_t Multiplier{0};
		Exchanges Exchange{ Exchanges::Smart };
		Exchanges PrimaryExchange{Exchanges::Smart}; // pick an actual (ie non-aggregate) exchange that the contract trades on.  DO NOT SET TO SMART.
		Proto::Currencies Currency{Proto::Currencies::NoCurrency};
		string LocalSymbol;
		string TradingClass;
		bool IncludeExpired{false};
		string SecIdType;		// CUSIP;SEDOL;ISIN;RIC
		string SecId;
		string ComboLegsDescrip; // received in open order 14 and up for all combos
		std::shared_ptr<std::vector<ComboLegPtr_>> ComboLegsPtr;
		DeltaNeutralContract DeltaNeutral;
		string Name;
		uint Flags{0};
		TimePoint IssueDate{ TimePoint::max() };
		ContractPK UnderlyingId{0};

		ContractPK ShortContractId()const noexcept;
		PositionAmount LongShareCount( Amount price )const noexcept;
		PositionAmount ShortShareCount( Amount price )const noexcept;
		PositionAmount RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept;
		Amount RoundDownToMinTick( Amount price )const noexcept;
		static DayIndex ToDay( str str )noexcept;
		sp<std::vector<Proto::Results::ContractHours>> TradingHoursPtr;//TODO a const vector.
		sp<std::vector<Proto::Results::ContractHours>> LiquidHoursPtr;

	};
	using ContractPtr_=sp<const Contract>;
	Φ Find( const std::map<ContractPK, ContractPtr_>&, sv symbol )noexcept->ContractPtr_;

	Φ ToIb( string symbol, Day dayIndex, SecurityRight right, double strike=0 )noexcept->::Contract;
	Φ ToProto( const ::ContractDetails& details )noexcept->Proto::Results::ContractDetail;
	namespace Contracts
	{
		ΓM extern const Contract Spy;
		ΓM extern const Contract SH;
		extern const Contract Qqq;
		extern const Contract Psq;
		ΓM extern const Contract Tsla;
		ΓM extern const Contract Aig;
		ΓM extern const Contract Xom;
	}
#pragma endregion
}
#undef Φ