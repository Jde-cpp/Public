#pragma once
#include "IMutationAwait.h"

namespace Jde::QL{
	struct AddRemoveAwait final: IMutationAwait{
		using base=IMutationAwait;
		using base::base;
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->TAwait<jvalue>::Task;//authorize, then the proc hook or the map-table statements between their hooks - one coroutine.
	};
}