#pragma once
#include "../uaTypes/Object.h"
#include <jde/opc/uatypes/NodeId.h>

namespace Jde::Opc::Server {
	struct ObjectAwait final : TAwaitEx<flat_map<NodePK, Object>,DB::SelectAwait::Task>{
		ObjectAwait( SRCE )ι:TAwaitEx<flat_map<NodePK, Object>,DB::SelectAwait::Task>{ sl }{};
	private:
		α Execute()ι->DB::SelectAwait::Task override;
	};
}