#pragma once
#include "../uaTypes/ObjectAttr.h"

namespace Jde::Opc::Server {
	struct ObjectAttrAwait final : TAwaitEx<flat_map<OAttrPK, ObjectAttr>,DB::SelectAwait::Task>{
		α Execute()ι->DB::SelectAwait::Task override;
	};
}