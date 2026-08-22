#pragma once
#include <jde/ql/LocalQL.h>

namespace Jde::App{
	struct AppQL : QL::LocalQL{
		AppQL(vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer)ι:QL::LocalQL{ move(schemas), authorizer }{};
		α LogQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->up<TAwait<jvalue>> override;
		α StatusQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->jobject override;
	protected:
		//The log/logSettings/status routes bypass SelectAwait's Authorize, so this is the only gate they have.  Authentication,
		//not authorization:  no acl resource exists for the log archive or the status document, and all three were anonymous.
		Ω RequireAuthenticated( QL::Creds& executer, sv what, SL sl )ε->void;
	};
}