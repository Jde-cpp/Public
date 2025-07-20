#pragma once
#include <jde/framework/coroutine/Await.h>
#include "../awaits/ConfigureAwait.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{  struct IQL; }
namespace Jde::Access{ struct Authorize; struct AccessListener; }

namespace Jde::Access::Client{
	α Configure( sp<DB::AppSchema> accessSchema, vector<sp<DB::AppSchema>>&& localSchemas, sp<QL::IQL> appQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait;
}
