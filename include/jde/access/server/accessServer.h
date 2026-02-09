#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/IQLSession.h>
#include "awaits/AuthenticateAwait.h"
#include "../awaits/ConfigureAwait.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; struct TableQL; struct MutationQL; }

namespace Jde::Access::Server{
	α Authenticate( str loginName, uint providerId, str opcServer={}, SRCE )ι->AuthenticateAwait;
	α Configure( vector<sp<DB::AppSchema>>&& schemas, sp<QL::LocalQL> localQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait;
	α CustomQuery( QL::TableQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>;
	α CustomMutation( QL::MutationQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>;
}