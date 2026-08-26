#pragma once
#include "../uaTypes/Variable.h"
#include <jde/opc/uatypes/NodeId.h>
#include "VariantAwait.h"

namespace Jde::Opc::Server {
	struct VariableAwait final : TAwaitEx<flat_map<VariablePK, Variable>,DB::SelectAwait::Task>{
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		α LoadVariants( vector<DB::Value>&& variants, vector<DB::Row> rows )ι->VariantMembersAwait::Task;
	};
}