#pragma once
#include <jde/blockly/Blockly.h>
#include <jde/blockly/types/EventResult.h>

namespace Jde::Markets::MBlockly::Blocks
{
	struct [[jde::ClassName]] final: Blockly
	{
		[[jde::ClassName]]( long orderId, uint32_t contractId )noexcept(false);
		void Run()noexcept override;
		Coroutine::Task<EventResult>  Run2()noexcept;
		bool Running()noexcept override{ return _pKeepAlive!=nullptr; }
[[jde::constexpr]]
	private:
[[jde::Prototypes]]
		void Assign( const EventResult& v )noexcept;
		Coroutine::Task<EventResult> Preliminary()noexcept;
		std::optional<ProcTimePoint> GetAlarm( ProcTimePoint now )noexcept(false);
		Markets::Tick::Fields TickFields()noexcept;
		uint _pass{0};
		sp<IBlockly> _pKeepAlive;
	};
}