#pragma once
#include <jde/blockly/IBlockly.h>
#include <jde/blockly/types/BTick.h>
#include <jde/blockly/Exports-Executor.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/types/proto/blocklyResults.pb.h>

namespace Jde::Markets{ struct Contract; struct TwsClientSync; }
namespace Jde::Markets::MBlockly
{
	struct ΓBE Blockly : IBlockly
	{
		Blockly( long orderId, uint32_t contractId )noexcept(false);
		α Available()noexcept(false)->Amount;
		α AccountNumber()const->const string&{ return Order.AccountNumber(); }
		α Currency()const->Markets::Proto::Currencies{ return Contract.Currency; }
	protected:
		β NoAsk()const noexcept(false)->Price;
		β NoBid()const noexcept(false)->Price;
		α PlaceOrder()noexcept(false)->void;
		α BumpBid()noexcept(false)->void;

		α Log( up<Proto::EventResult> er )->void;
		α LogPlaceOrder( Price limit )->void;
		α Log( const IException& e )->void;
		α LogExit()->void;
		ProcOrder Order;
		OrderStatus Status;
		BTick Tick;
		MBlockly::Account Account;
		MBlockly::Limits Limits;
		sp<TwsClientSync> TwsPtr;
		Markets::Contract Contract;
	private:
		α Log( function<void(Proto::Entry&)> f )->void;
		
		vector<Proto::FileEntry> _logEntries;
	};
}