#pragma once
#include <jde/app/AppQL.h>

namespace Jde::Opc::Gateway{
	struct GatewayQL;
	α QLPtr()ι->sp<GatewayQL>;
	α QL()ι->GatewayQL&;
	α ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;

	struct GatewayQL : App::AppQL{
		GatewayQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι;
		α CustomQuery( QL::TableQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>> override;
		α CustomMutation( QL::MutationQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>> override;
	};
}