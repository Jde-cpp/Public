#pragma once
#include <jde/db/graphQL/GraphQLHook.h>
#include "exports.h"

namespace Jde::Web::Server{
	struct ΓWS SessionGraphQL : DB::GraphQL::IGraphQLHook{
		α Select( const DB::TableQL& query, UserPK userPK, SRCE )ι->up<IAwait> override;
	};
}