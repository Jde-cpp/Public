#pragma once
#include <jde/ql/LocalQL.h>

namespace Jde::App::Server{
	α QLPtr()ι->sp<QL::LocalQL>;
	α QL()ι->QL::LocalQL&;
	α ConfigureQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι->void;

	struct AppQL : QL::LocalQL{
		AppQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer )ι;
		α CustomQuery( QL::TableQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
		α CustomMutation( QL::MutationQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
	};
}