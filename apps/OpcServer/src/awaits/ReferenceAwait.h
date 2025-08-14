#pragma once
#include "../uaTypes/Reference.h"

namespace Jde::Opc::Server {
	struct ReferenceAwait final : TAwaitEx<flat_map<NodePK,Reference>,DB::SelectAwait::Task>{
		α Execute()ι->DB::SelectAwait::Task override;
	};

	struct ReferenceInsertAwait final : TAwaitEx<PK,DB::ScalerAwait<uint>::Task>{
		using base = TAwaitEx<Variable,DB::ScalerAwait<uint>::Task>;
		ReferenceInsertAwait( Variable&& node, UserPK executer, SL sl )ι:base{ sl }, _node{ move(node) }, _executer{ executer } {}
	private:
		α Execute()ι->DB::ScalerAwait<uint>::Task override;
		α InsertMembers( DB::Value variantPK )ι->DB::ExecuteAwait::Task;
		Variable _node;
	};
}