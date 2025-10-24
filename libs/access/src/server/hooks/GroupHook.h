#pragma once
#include <jde/ql/QLHook.h>
#include <jde/access/usings.h>

namespace Jde::Access::Server{
	struct GroupHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK executer, SRCE )ι->HookResult override;
		α AddBefore( const QL::MutationQL& m, jobject variables, UserPK executer, SRCE )ι->HookResult override;
	private:
		α AddRemoveArgs( const QL::MutationQL& m )ι->std::pair<GroupPK, flat_set<IdentityPK::Type>>;
	};
}