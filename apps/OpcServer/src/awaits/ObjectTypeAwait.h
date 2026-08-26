#pragma once
#include "../uaTypes/ObjectType.h"

namespace Jde::Opc::Server {
	struct ObjectTypeAwait final : TAwaitEx<flat_map<NodePK, sp<ObjectType>>,DB::SelectAwait::Task>{
	private:
		α Execute()ι->DB::SelectAwait::Task override;
	};
}