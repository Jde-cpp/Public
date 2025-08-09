#pragma once
#include <jde/ql/QLHook.h>

namespace Jde::App{ struct IApp; }
namespace Jde::App::Server{
	struct AppInstanceHook final : QL::IQLHook{
		AppInstanceHook( sp<IApp> appClient )ι: _appClient{move(appClient)}{}
		α Start( const QL::MutationQL& mu, UserPK userPK, SRCE )ι->HookResult;
		α Stop( const QL::MutationQL& mu, UserPK userPK, SRCE )ι->HookResult;
	private:
		sp<IApp> _appClient;
	};
}