#pragma once
//#include "types/Proc.h"
//#include "types/BTick.h"
//#include <jde/coroutine/Task.h>
//#include "Exports-Executor.h" abstract class, should not need it

//namespace Jde::Markets{ struct Contract; }

namespace Jde::Markets::MBlockly
{
	struct IBlockly : std::enable_shared_from_this<IBlockly>
	{
		virtual ~IBlockly()=default;
		virtual void Run()noexcept=0;
		virtual bool Running()noexcept=0;
	};
}
