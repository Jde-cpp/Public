#pragma once
#include <jde/ql/GraphQL.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/metadata/Table.h>

namespace Jde::QL{
	struct PurgeAwait final: AsyncAwait{
		PurgeAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK, sp<DB::IDataSource> ds, SRCE )ι;
		α Execute( const DB::Table& table, MutationQL m, UserPK userId, sp<DB::IDataSource> ds, HCoroutine h )ι->Task;
	};
}