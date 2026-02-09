#pragma once
#include <jde/ql/LocalQL.h>

namespace Jde::App{
	struct AppQL : QL::LocalQL{
		AppQL(vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer)ι:QL::LocalQL{ move(schemas), authorizer }{};
		α LogQuery( QL::TableQL&& ql, SL sl )ι->up<TAwait<jvalue>> override;
		α StatusQuery( QL::TableQL&& ql )ι->jobject override;
	};
}