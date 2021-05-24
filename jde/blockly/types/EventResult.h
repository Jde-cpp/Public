#pragma once
#include "BTick.h"
namespace Jde::Markets::MBlockly
{
	struct EventResult
	{
		MBlockly::ProcOrder Order;
		Markets::Tick Tick;
		Jde::Markets::OrderStatus Status;
	};
}