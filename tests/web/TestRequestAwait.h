#pragma once
#include <jde/web/flex/HttpRequestAwait.h>
#include <jde/web/flex/Flex.h>

namespace Jde::Web{
	constexpr sv Host{ "localhost" };
	constexpr sv Port{ "5005" };
	α StartTestServer()ι->void;
	α StopTestServer()ι->void;
}
namespace Jde::Web::Flex{

	struct TestRequestAwait : IHttpRequestAwait{
		using base = IHttpRequestAwait;
		TestRequestAwait( HttpRequest&& req, SRCE )ι;
		α await_ready()ι->bool override;
		α await_suspend( HttpCo h )ε->void;
		α await_resume()ε->HttpTaskResult override;
	private:
		optional<HttpTaskResult> _result;
		optional<std::jthread> _thread;
	};

	struct TestRequestHandler final : RequestHandler{
		α HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> override{ return mu<TestRequestAwait>( move(req), sl ); }
	};
}