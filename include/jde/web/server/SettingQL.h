#pragma once
#include <jde/ql/QLHook.h>
#include "exports.h"

namespace Jde::App{ struct IApp; }
namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct ΓWS SettingQL : QL::IQLHook{
		SettingQL( sp<App::IApp> appClient )ι:_appClient{move(appClient)}{}
		α Select( const QL::TableQL& query, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	private:
		sp<App::IApp> _appClient;
	};
}