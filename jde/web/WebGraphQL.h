#pragma once
#include <jde/db/graphQL/GraphQLHook.h>

namespace Jde::Web{
	struct WebGraphQL : DB::GraphQL::IGraphQLHook{
		α Select( const DB::TableQL& query, UserPK userPK, SRCE )ι->up<IAwait> override;
	};
}