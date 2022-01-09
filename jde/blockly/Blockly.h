#pragma once
#include <jde/blockly/IBlockly.h>
#include <jde/blockly/types/BTick.h>
#include <jde/blockly/Exports-Executor.h>
#include <jde/markets/types/Contract.h>

namespace Jde::Markets{ struct Contract; struct TwsClientSync; }
namespace Jde::Markets::MBlockly
{
	struct JDE_BLOCKLY_EXECUTOR Blockly : IBlockly
	{
		Blockly( long orderId, uint32_t contractId )noexcept(false);
		Amount Available()noexcept(false);
		const string& AccountNumber()const{ return Order.AccountNumber(); }
		Proto::Currencies Currency()const{ return Contract.Currency; }
	protected:
		virtual Price NoAsk()const noexcept(false);
		virtual Price NoBid()const noexcept(false);
		void PlaceOrder()noexcept(false);

		ProcOrder Order;
		OrderStatus Status;
		BTick Tick;
		MBlockly::Account Account;
		MBlockly::Limits Limits;
		sp<TwsClientSync> TwsPtr;
		Markets::Contract Contract;
		void BumpBid()noexcept(false);
	};
}