#pragma once

#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/access/awaits/AuthenticateAwait.h>
#include "awaits/ConfigureAwait.h"
#include "exports.h"
#include "usings.h"
//#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::QL{ struct MutationQL; enum class EMutationQL : uint8; }

#define Φ ΓA auto
namespace Jde::Access{
	struct IAcl;
	//schemas=empty for all.
	Φ Configure( sp<DB::AppSchema> access, vector<sp<DB::AppSchema>>&& schemas, sp<QL::IQL> qlServer, UserPK executer )ε->ConfigureAwait;
	α LocalAcl()ι->sp<IAcl>;
	α Authenticate( str loginName, uint providerId, str opcServer={}, SRCE )ι->AuthenticateAwait;
}
#undef Φ
