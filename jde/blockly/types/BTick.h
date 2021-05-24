#pragma once
#include <jde/markets/types/Tick.h>
#include "Proc.h"
#include "../Exports-Executor.h"

namespace Jde::Markets::MBlockly
{
	struct Awaitable; struct AwaitableShim;
	struct JDE_BLOCKLY_EXECUTOR BTick : private Tick
	{
		BTick( ContractPK id ):Tick{id}{}
		BTick( const Tick& tick  ):Tick{tick}{}
		BTick( const BTick& rhs ):Tick{rhs}/*, NoAsk{NotSet}, NoBid{NotSet}*/{}
		/*BTick( Tick&& tick  ):Tick{std::move(tick)}{}*/
		//void SetNoAsk( std::function<Price()> fnctn )const noexcept{ NoAsk = fnctn; };
		Price AskPrice()const noexcept(false);
		//void SetNoBid( std::function<Price()> fnctn )const noexcept{ NoBid = fnctn; };
		Price BidPrice()const noexcept(false);
		Price Spread()const noexcept(false){ return AskPrice()-BidPrice(); }
		Size BidSize()const noexcept(false);
		Size AskSize()const noexcept(false);
		bool AllSet( Markets::Tick::Fields fields );
		BTick& operator=( const Tick& rhs )noexcept;
		//void Assign( const Tick& rhs )noexcept{ (Tick&)*this=rhs; }

//TODO Bid
	private:
		//BTick( Tick&& t ):Tick{ std::move(t) }{ DBG0("BTick(Tick&& t)"sv); }
		/*[[noreturn]]*/ static Price NotSet(){ return Price{0.0}; /*Price{NAN}TODO*/ }
		//mutable std::function<Price()> NoAsk{ NotSet };
		//mutable std::function<Price()> NoBid{ NotSet };
		const Tick& Base()const noexcept{ return static_cast<const Tick&>(*this); }

		friend Awaitable; friend AwaitableShim;
	};
}