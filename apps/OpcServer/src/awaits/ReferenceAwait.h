#pragma once
#include "../uaTypes/Reference.h"

namespace Jde::Opc::Server {
	struct ReferenceAwait final : TAwaitEx<flat_map<NodePK,Reference>,DB::SelectAwait::Task> {
		α Execute()ι->DB::SelectAwait::Task override;
	};
}