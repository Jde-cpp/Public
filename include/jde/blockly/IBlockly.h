#pragma once
#include <memory>

namespace Jde::Markets::MBlockly
{
	struct IBlockly : std::enable_shared_from_this<IBlockly>
	{
		virtual ~IBlockly()=default;
		virtual void Run()noexcept=0;
		virtual bool Running()noexcept=0;
	};
}