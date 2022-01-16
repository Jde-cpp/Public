#pragma once
#include "Exports.h"
#include <jde/coroutine/Task.h>
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#include "./types/proto/blockly.pb.h"
#pragma warning( default : 4996 )
#pragma warning( default : 4244 )


#define Φ JDE_BLOCKLY auto
namespace Jde::Markets
{
	struct MyOrder; struct Tick; struct Account; struct Contract; struct TwsClient;
	namespace MBlockly{ struct Account; struct IBlockly; }
	constexpr array<sv,25> TickTypeStrings = { "BidSize"sv, "BidPrice"sv, "AskPrice"sv, "AskSize"sv, "LastPrice"sv, "LastSize"sv, "High"sv, "Low"sv, "Volume"sv, "ClosePrice"sv, "BID_OPTION_COMPUTATION"sv, "ASK_OPTION_COMPUTATION"sv, "LAST_OPTION_COMPUTATION"sv, "MODEL_OPTION"sv, "OpenTick"sv, "Low13Week", "High13Week", "Low26Week", "High26Week", "Low52Week", "High52Week", "AverageVolume", "OPEN_INTEREST", "OptionHistoricalVol", "OptionImpliedVol" };
}

namespace Jde::Blockly
{
	static constexpr uint32 IdenticalCrcLogId = 2152742774;

	Φ SetBasePath( path path )noexcept->void;
	Φ Copy( sv from, Proto::Function to )noexcept(false)->void;
	Φ Save( const Proto::Function& fnctn )noexcept(false)->void;
	Φ Delete( sv id )noexcept(false)->void;
	Φ Load()noexcept(false)->up<Proto::Functions>;
	Φ Load( sv id, bool lock=true )noexcept(false)->up<Proto::Function>;
	Φ Build( str id )noexcept(false)->void;
	Φ DeleteBuild( sv id )noexcept(false)->void;
	Φ Enable( str id )noexcept(false)->void;
	Φ Disable( sv id )noexcept(false)->void;
	Φ CreateAllocatedExecutor( sv blockId, long orderId, uint32_t contractId )noexcept(false)->sp<Markets::MBlockly::IBlockly>*;
	//Φ Execute( std::string_view id, const Markets::MyOrder& order/*, const Jde::Markets::Tick& tick, const Jde::Markets::MBlockly::Account& account*/, const Jde::Markets::Contract& contract, std::shared_ptr<Jde::Markets::TwsClient> pTws )noexcept(false);
	//Φ Execute( std::string_view id, const Markets::MyOrder& order, const Jde::Markets::Contract& contract, std::shared_ptr<Jde::Markets::TwsClient> pTws )noexcept(false);
}
#undef Φ