#pragma once
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::App::Server{
	struct HttpRequestAwait final: Web::Server::IHttpRequestAwait{
		using base = Web::Server::IHttpRequestAwait;
		HttpRequestAwait( Web::Server::HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void;
		α await_resume()ε->Web::Server::HttpTaskResult override;
	private:
		α QueryHandler( QL::RequestQL&& q, variant<sp<Web::Server::SessionInfo>, Jde::UserPK> creds, bool returnRaw, SRCE )ι->up<Web::Server::IQLAwait>;
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&;
	};
}
