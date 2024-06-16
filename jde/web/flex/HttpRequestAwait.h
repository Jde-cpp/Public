#pragma once
#include <jde/web/usings.h>

#define Φ Γ auto
namespace Jde::Web::Flex{
	struct HttpTask{
		struct promise_type{
			promise_type()ι{}

			α get_return_object()ι->HttpTask{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			Φ unhandled_exception()ι->void;

			α SetResult( http::message_generator&& m )ι->void{ _result = move(m); }
		private:
			optional<http::message_generator> _result;
			source_location _sl;
		};
	};
	using HttpCo = coroutine_handle<HttpTask::promise_type>;
	struct IHttpRequestAwait {
	};

	using TBody=http::string_body;
	using TAllocator=std::allocator<char>;
//	template<class TBody, class TAllocator>
	struct HttpRequestAwait : net::awaitable<http::message_generator, net::any_io_executor>{
		HttpRequestAwait( http::request<TBody, http::basic_fields<TAllocator>>&& req, SRCE )ι:_req{ move(req) }{}

		β await_ready()ι->bool;
		β await_suspend( HttpCo h )ι->void;
		β await_resume()ι->http::message_generator;
	private:
		HttpTask::promise_type* _pPromise{};
		http::request<TBody, http::basic_fields<TAllocator>> _req;
	};
}
#undef Φ