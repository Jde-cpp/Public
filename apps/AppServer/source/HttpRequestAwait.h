#pragma once
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::App{
	using namespace Jde::Web::Server;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void;
		α await_resume()ε->HttpTaskResult override;
	};
}
