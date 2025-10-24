#pragma once
#include <jde/ql/QLHook.h>
#include "exports.h"

namespace Jde::App{ struct IApp; }
namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct ΓWS SessionGraphQL : QL::IQLHook{
		SessionGraphQL( sp<App::IApp> _appClient	)ι: _appClient{move(_appClient)}{}
		α Select( const QL::TableQL& query, UserPK userPK, SRCE )ι->HookResult override;
		β PurgeBefore( const QL::MutationQL&, jobject variables, UserPK, SRCE )ι->HookResult override;
	private:
		sp<App::IApp> _appClient;
	};
}