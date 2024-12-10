#pragma once
#include <jde/ql/QLHook.h>
#include <jde/access/usings.h>

namespace Jde::Access{
	struct GroupHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SRCE )ι->HookResult override;
		α AddAfter( const QL::MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult override;
		α RemoveAfter( const QL::MutationQL& mutation, UserPK userPK, SRCE )ι->HookResult override;
	private:
		α AddRemoveArgs( const QL::MutationQL& mutation )ι->std::pair<GroupPK, flat_set<IdentityPK>>;
	};
}