#pragma once
#include <jde/web/server/IHttpRequestAwait.h>
//#include <jde/web/flex/Flex.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Server;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α await_suspend( base::Handle h )ε->void override;
		α await_resume()ε->HttpTaskResult override;
	private:
		optional<HttpTaskResult> _result;
		optional<std::jthread> _thread;
	};
}
