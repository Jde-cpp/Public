#pragma once
#include <jde/ql/LocalQL.h>

namespace Jde::Access::Tests{
	struct AccessQL : QL::LocalQL{
		AccessQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer )ι;
		α CustomQuery( QL::TableQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
		α CustomMutation( QL::MutationQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
	};
}