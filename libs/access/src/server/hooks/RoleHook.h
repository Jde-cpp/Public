#pragma once
#include <jde/ql/QLHook.h>

namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Access::Server{
	struct RoleHook final : QL::IQLHook{
		α Select( const QL::TableQL&, UserPK, SRCE )ι->HookResult override;
		α Add( const QL::MutationQL& m, UserPK userPK, SRCE )ι->HookResult override;
		α Remove( const QL::MutationQL& m, UserPK userPK, SRCE )ι->HookResult override;
	};
}