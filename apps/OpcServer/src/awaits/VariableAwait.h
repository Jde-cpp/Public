#pragma once
#include "../uaTypes/Variable.h"
#include <jde/opc/uatypes/Node.h>
#include "VariantAwait.h"

namespace Jde::Opc::Server {
	struct VariableAwait final : TAwaitEx<flat_map<VariablePK, Variable>,DB::SelectAwait::Task>{
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		α LoadVariants( vector<DB::Value>&& variants, vector<DB::Row> rows )ι->VariantMembersAwait::Task;
	};

	struct VariableInsertAwait final : TAwaitEx<Variable,DB::ScalerAwait<uint>::Task>{
		using base = TAwaitEx<Variable,DB::ScalerAwait<uint>::Task>;
		VariableInsertAwait( Variable&& node, SRCE )ι;
	private:
		α Execute()ι->DB::ScalerAwait<uint>::Task override;
		α InsertMembers( DB::Value variantPK )ι->DB::ExecuteAwait::Task;
		Variable _node;
	};
}