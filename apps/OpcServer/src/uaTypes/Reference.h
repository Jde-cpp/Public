#pragma once
#include "Node.h"

namespace Jde::Opc::Server {
	struct Reference final{
		Reference( DB::Row&& r )ι:
			SourcePK{ r.GetUInt32(0) },
			TargetPK{ r.GetUInt32(1) },
			TypePK{ r.GetUInt32(2) },
			IsForward{ r.GetBitOpt(3).value_or(true) }
		{}

		NodePK SourcePK;
		NodePK TargetPK;
		NodePK TypePK;
		bool IsForward;
	};
}