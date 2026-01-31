#pragma once
#include <jde/ql/types/RequestQL.h>
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::App::Server{
	struct AppQLAwait final : Web::Server::IQLAwait{
		using base = Web::Server::IQLAwait;
		AppQLAwait( QL::RequestQL&& ql, variant<sp<Web::Server::SessionInfo>, Jde::UserPK> creds, bool raw, SRCE )ι:base{move(ql),move(creds), raw, sl}{}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
	};
}