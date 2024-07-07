#pragma once
#include <jde/db/graphQL/GraphQLHook.h>

namespace Jde::App::Client{
	struct SessionGraphQL : DB::GraphQL::IGraphQLHook{
		α Select( const DB::TableQL& query, UserPK userPK, SRCE )ι->up<IAwait> override;
	};
}