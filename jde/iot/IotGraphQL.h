#pragma once
#include <jde/db/graphQL/GraphQLHook.h>

namespace Jde::Iot{
	struct IotGraphQL : DB::GraphQL::IGraphQLHook{
		Ω CreateProvider()ι;
		α InsertBefore( const DB::MutationQL& mutation, SRCE )ι->up<IAwait> override;
		α InsertFailure( const DB::MutationQL& mutation, SRCE )ι->up<IAwait> override;
		α PurgeBefore( const DB::MutationQL& mutation, SRCE )ι->up<IAwait> override;
		α PurgeFailure( const DB::MutationQL& mutation, SRCE )ι->up<IAwait> override;
	};
}