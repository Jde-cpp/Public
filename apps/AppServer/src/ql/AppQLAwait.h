#pragma once
#include <jde/ql/types/RequestQL.h>
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/ql/IQLAwaitExe.h>

namespace Jde::App::Server{
	struct AppQLAwait final : QL::IQLTableAwaitExe{
		Ω Test( QL::TableQL& q, QL::Creds& executer, SL sl )->up<TAwait<jvalue>>;
	};
}