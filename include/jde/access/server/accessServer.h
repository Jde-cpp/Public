#pragma once
#include <jde/framework/coroutine/Await.h>
#include "awaits/AuthenticateAwait.h"
#include "../awaits/ConfigureAwait.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; }

namespace Jde::Access::Server{
	//α AccessQL()ε->sp<QL::LocalQL>;
	α Authenticate( str loginName, uint providerId, str opcServer={}, SRCE )ι->AuthenticateAwait;
	//α Authorizer()ε->sp<Access::Authorize>;
	α Configure( vector<sp<DB::AppSchema>>&& schemas, sp<QL::LocalQL> localQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait;
}