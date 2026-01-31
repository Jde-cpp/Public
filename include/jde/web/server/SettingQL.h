#pragma once
#include <jde/ql/QLHook.h>
#include "exports.h"

namespace Jde::App{ struct IApp; }
namespace Jde::QL{ struct TableQL; }
namespace Jde::Web::Server{
	struct SettingQLAwait final : TAwait<jvalue>{
		SettingQLAwait( const QL::TableQL& query, sp<App::IApp> appClient, SL sl )ι: TAwait<jvalue>{ sl }, _appClient{move(appClient)}, _query{ query }{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ ASSERT(false); }
		α await_resume()ε->jvalue override;
	private:
		sp<App::IApp> _appClient;
		up<exception> _exception;
		QL::TableQL _query;
		jvalue _result;
	};
}