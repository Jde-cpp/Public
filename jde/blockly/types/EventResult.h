#pragma once
#include "BTick.h"
#include <jde/blockly/Exports-Executor.h>
#include <jde/markets/types/proto/blocklyResults.pb.h>

namespace Jde::Markets::MBlockly
{
	struct EventResult
	{
		using ProtoResult=Proto::EventResult;
		α ToProto()Ι->up<ProtoResult>;
		MBlockly::ProcOrder Order;
		Markets::Tick Tick;
		OrderStatus Status;
	};

	Ξ EventResult::ToProto()Ι->up<ProtoResult>
	{
		auto y{ mu<ProtoResult>() };
		y->set_allocated_order( Order.ToProto().release() );
		y->set_allocated_tick( Tick.ToProto().release() );
		y->set_allocated_status( Status.ToProto().release() );
		return y;
	}
}