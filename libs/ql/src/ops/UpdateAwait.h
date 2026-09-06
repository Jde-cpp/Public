#pragma once
#include "IMutationAwait.h"

namespace Jde::DB{ struct Table; struct UpdateClause; }
namespace Jde::QL{
	struct UpdateAwait final: IMutationAwait{
		using base=IMutationAwait;
		using base::base;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ Execute(); }
	private:
		α CreateDeleteRestore( const DB::Table& table, const jobject& input )ε->DB::UpdateClause;
		α Execute()ι->TAwait<jvalue>::Task;//the enum prefetch, the clauses, UpdateBefore, the statements, UpdateAfter - one coroutine.
	};
}