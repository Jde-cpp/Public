#pragma once
#include "../uaTypes/Reference.h"

namespace Jde::Opc::Server {
	struct ReferenceAwait final : TAwaitEx<flat_map<NodePK,Reference>,DB::SelectAwait::Task>{
		α Execute()ι->DB::SelectAwait::Task override;
	};

	struct ReferenceInsertAwait final : VoidAwait{
		ReferenceInsertAwait( Reference&& reference, SL sl )ι:VoidAwait{ sl }, _ref{ move(reference) }{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<uint>::Task;
		Reference _ref;
	};
}