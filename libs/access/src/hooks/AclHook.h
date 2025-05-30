#pragma once
#include <jde/access/usings.h>
#include <jde/ql/QLHook.h>
//#include "Permission.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Access{
	struct AclHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SL sl )ι->HookResult override;
		α PurgeBefore( const QL::MutationQL&, UserPK, SL sl )ι->HookResult  override;
		α InsertBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult override;
	};
}