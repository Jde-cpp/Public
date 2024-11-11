#pragma once
#include <jde/ql/GraphQLHook.h>
#include "exports.h"

namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct ΓWS SessionGraphQL : QL::IGraphQLHook{
		α Select( const QL::TableQL& query, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};
}