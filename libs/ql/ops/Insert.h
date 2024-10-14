#pragma once
#include <jde/ql/GraphQL.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::DB{ struct Table; struct IDataSource; }
namespace Jde::QL{
	struct InsertAwait final: AsyncReadyAwait{
		InsertAwait( const DB::Table& table, const MutationQL& mutation, UserPK userPK, uint extendedFromId, sp<DB::IDataSource> ds, SRCE )ι;
		α CreateQuery( const DB::Table& table, uint extendedFromId )ι->optional<AwaitResult>;
		α Execute( HCoroutine h, UserPK userPK )ι->Task;
	private:
		sp<DB::IDataSource> _ds;
		const MutationQL _mutation;
		vector<DB::Value> _parameters;
		std::ostringstream _sql;
	};
}