#pragma once
#include <jde/ql/QLHook.h>

namespace Jde::QL{ struct MutationQL; struct TableQL; }
namespace Jde::Opc::Server{
	struct ObjectTypeHook final : QL::IQLHook{
		α InsertBefore( const QL::MutationQL& m, jobject variables, UserPK executer, SL sl )ι->HookResult override;
	};
}