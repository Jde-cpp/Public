#pragma once
#include <jde/ql/QLHook.h>
#include "exports.h"

namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct ΓWS SessionGraphQL : QL::IQLHook{
		α Select( const QL::TableQL& query, UserPK userPK, SRCE )ι->HookResult override;
		β PurgeBefore( const QL::MutationQL&, UserPK, SRCE )ι->HookResult override;
	};
}