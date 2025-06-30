#pragma once
#include "../uaTypes/ObjectType.h"

namespace Jde::Opc::Server {
	struct ObjectTypeAwait final : TAwaitEx<flat_map<NodePK, sp<ObjectType>>,DB::SelectAwait::Task>{
	private:
		α Execute()ι->DB::SelectAwait::Task override;
	};

	struct ObjectTypeInsertAwait final : TAwaitEx<sp<ObjectType>,DB::ScalerAwait<NodePK>::Task>{
		using base = TAwaitEx<sp<ObjectType>,DB::ScalerAwait<NodePK>::Task>;
		ObjectTypeInsertAwait( sp<ObjectType> node, SRCE )ι;
	private:
		α Execute()ι->DB::ScalerAwait<NodePK>::Task override;
		sp<ObjectType> _node;
	};
}