#pragma once
#include <jde/web/flex/IHttpRequestAwait.h>
//#include <jde/web/flex/Flex.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;
	struct HttpRequestAwait final: IHttpRequestAwait{
		using base = IHttpRequestAwait;
		HttpRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α await_suspend( HttpCo h )ε->void;
		α await_resume()ε->HttpTaskResult override;
	private:
		optional<HttpTaskResult> _result;
		optional<std::jthread> _thread;
	};
}
