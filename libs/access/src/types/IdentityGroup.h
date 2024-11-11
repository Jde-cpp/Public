#pragma once
#include "IAccessIdentity.h"
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/GraphQLHook.h>

namespace Jde::DB{ struct AppSchema; }

namespace Jde::Access{
	struct Group final : IAccessIdentity{
		Group( GroupPK id )ι:Id(id){}

		GroupPK Id;
	};

	struct GroupLoadAwait final : TAwaitEx<concurrent_flat_map<GroupPK,Group>,Coroutine::Task>{
		GroupLoadAwait( sp<DB::AppSchema> schema, UserPK userPK )ι;
	private:
		α Execute()ι->Coroutine::Task override;
		sp<DB::AppSchema> _schema;
		UserPK _userPK;
	};

	struct GroupGraphQL final : QL::IGraphQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};

}