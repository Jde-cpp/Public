#pragma once
#include "Node.h"

namespace Jde::Opc::Server{
	struct Reference final{
		Reference( DB::Row&& r )ι:
			SourcePK{ r.GetUInt32(0) },
			TargetPK{ r.GetUInt32(1) },
			RefTypePK{ r.GetUInt32(2) },
			IsForward{ r.GetBitOpt(3).value_or(true) }
		{}

		Reference( const jobject& o )ι:
			SourcePK{ Json::FindNumberPath<NodePK>(o, "source/id").value_or(0) },
			TargetPK{ Json::FindNumberPath<NodePK>(o, "target/id").value_or(0) },
			RefTypePK{ Json::FindNumberPath<NodePK>(o, "refType/id").value_or(0) },
			IsForward{ Json::FindBool(o, "isForward").value_or(true) }
		{}

		NodePK SourcePK;
		NodePK TargetPK;
		NodePK RefTypePK;
		bool IsForward;
	};
}