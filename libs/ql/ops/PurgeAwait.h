#pragma once
#include "../GraphQL.h"
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/meta/Table.h>

namespace Jde::QL{
	struct PurgeAwait final: AsyncAwait{
		PurgeAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK, SRCE )ι;
		α Execute( const DB::Table& table, MutationQL m, UserPK userId, HCoroutine h )ι->Task;
	};
}