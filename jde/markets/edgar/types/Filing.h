#pragma once
#include <jde/TypeDefs.h>
#include "../Exports.h"
#include <jde/markets/types/proto/edgar.pb.h>

namespace Jde::Markets::Edgar
{
	struct Filing
	{
		friend α operator<(const Filing& aFiling, const Filing& bFiling)noexcept->bool;
		Ω FileName( const Proto::Filing& filing )noexcept->string;
		α FileName()const noexcept->string;
		α Line()const noexcept->uint32{ return Proto.line_number(); }
		Proto::Filing Proto;
	};
}