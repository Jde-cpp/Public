#pragma once
#include "../uaTypes/VariableAttr.h"
#include "VariantAwait.h"

namespace Jde::Opc::Server {
	struct VariableAttrAwait final : TAwaitEx<flat_map<VAttrPK, VariableAttr>,DB::SelectAwait::Task> {
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		α LoadVariants( vector<DB::Value>&& variants, vector<DB::Row>&& rows )ι->VariantAwait::Task;
	};
}