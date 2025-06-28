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

	struct VariableInsertAwait final : TAwaitEx<Variable,DB::ScalerAwait<uint32>::Task>{
		using base = TAwaitEx<Variable,DB::ScalerAwait<uint32>::Task>;
		VariableInsertAwait( Variable&& node, SRCE )ι;
	private:
		α Execute()ι->DB::ScalerAwait<uint32>::Task override;
		Variable _node;
	};
}