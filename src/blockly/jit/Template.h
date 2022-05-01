#pragma once
#include <jde/blockly/Blockly.h>
#include <jde/blockly/types/EventResult.h>
#include <jde/blockly/EventManager.h>
#include <jde/coroutine/Task.h>

namespace Jde::Markets::MBlockly::Blocks
{
	struct [[jde::ClassName]] final: Blockly
	{
		[[jde::ClassName]]( long orderId, uint32_t contractId )ε;
		α Run()ι->void override;
		α Run2()ι->BTask<EventResult>;
		α Running()ι->bool override{ return _pKeepAlive!=nullptr; }
[[jde::constexpr]]
	private:
[[jde::Prototypes]]
		α Assign( const EventResult& v )ι->void;
		α Preliminary()ι->Coroutine::Task;
		α GetAlarm( ProcTimePoint now )ε->std::optional<ProcTimePoint>;
		α TickFields()ι->Markets::Tick::Fields;
		α TickTrigger()ι->optional<function<bool(const Markets::Tick&)>>;

		uint _pass{0};
		sp<IBlockly> _pKeepAlive;
	};
}