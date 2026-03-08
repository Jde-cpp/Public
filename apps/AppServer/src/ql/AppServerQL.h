#pragma once
#include <jde/ql/LocalQL.h>
#include <jde/app/AppQL.h>

namespace Jde::App::Server{
	struct AppServerQL;
	α QLPtr()ι->sp<QL::LocalQL>;
	α QL()ι->QL::LocalQL&;
	α ConfigureQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι->void;

	struct AppServerQL : App::AppQL{
		AppServerQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize>&& authorizer )ι;
		α CustomQuery( QL::TableQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>> override;
		α CustomMutation( QL::MutationQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>> override;
		α LogSettingsQuery( QL::TableQL&& ql, SL sl )ι->up<TAwait<jvalue>> override;
	};
}