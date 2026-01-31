#pragma once
#include <jde/ql/LocalQL.h>

namespace Jde::Opc::Server{
	struct OpcQL;
	α QLPtr()ι->sp<OpcQL>;
	α QL()ι->OpcQL&;
	α ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;

	struct OpcQL : QL::LocalQL{
		OpcQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι;
		α CustomQuery( QL::TableQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
		α CustomMutation( QL::MutationQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override;
	};
}