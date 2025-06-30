#pragma once
#include "../uaTypes/Node.h"

namespace Jde::Opc::Server {
	struct NodeAwait final : TAwaitEx<flat_map<NodePK,NodeId>,DB::SelectAwait::Task>{
		α Execute()ι->DB::SelectAwait::Task override;
	};
}