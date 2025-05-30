#pragma once
#include <jde/ql/QLHook.h>
#include "exports.h"

namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct ΓWS SettingQL : QL::IQLHook{
		α Select( const QL::TableQL& query, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};
}