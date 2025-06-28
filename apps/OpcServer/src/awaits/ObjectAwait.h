#pragma once
#include "../uaTypes/Object.h"
#include <jde/opc/uatypes/Node.h>

namespace Jde::Opc::Server {
	struct ObjectAwait final : TAwaitEx<flat_map<VariablePK, Object>,DB::SelectAwait::Task>{
		ObjectAwait( SRCE )ι:TAwaitEx<flat_map<VariablePK, Object>,DB::SelectAwait::Task>{ sl }{};
	private:
		α Execute()ι->DB::SelectAwait::Task override;
	};

	struct ObjectInsertAwait final : TAwaitEx<Object,DB::ScalerAwait<NodePK>::Task>{
		using base = TAwaitEx<Object,DB::ScalerAwait<NodePK>::Task>;
		ObjectInsertAwait( Object&& node, SRCE )ι;
	private:
		α Execute()ι->DB::ScalerAwait<NodePK>::Task override;
		Object _node;
	};
}