#pragma once
#include <jde/ql/QLHook.h>

namespace Jde::QL{ struct MutationQL; }
namespace Jde::Opc::Server{
	struct ObjectHook final : QL::IQLHook{
		α InsertBefore( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult override;
	};
}