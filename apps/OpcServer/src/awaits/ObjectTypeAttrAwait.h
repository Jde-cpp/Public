#pragma once
#include "../uaTypes/ObjectTypeAttr.h"

namespace Jde::Opc::Server {
	struct ObjectTypeAttrAwait final : TAwaitEx<flat_map<OTypeAttrPK,ObjectTypeAttr>,DB::SelectAwait::Task>{
		α Execute()ι->DB::SelectAwait::Task override;
	};
}