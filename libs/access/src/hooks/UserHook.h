#include <jde/ql/QLHook.h>

namespace Jde::Access{
	struct UserHook final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};
}