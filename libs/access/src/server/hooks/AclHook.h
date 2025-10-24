#pragma once
#include <jde/access/usings.h>
#include <jde/ql/QLHook.h>

namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Access::Server{
	struct AclHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SL sl )ι->HookResult override;
		α PurgeBefore( const QL::MutationQL&, jobject variables, UserPK, SL sl )ι->HookResult  override;
		α InsertBefore( const QL::MutationQL& m, jobject variables, UserPK userPK, SL sl )ι->HookResult override;
	};
}