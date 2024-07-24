#pragma once
#include "usings.h"
#include "Streams.h"
#include "HttpRequest.h"
#include "RestException.h"

#define Φ ΓWS auto

namespace Jde::Web::Server{
	using TRequestType = http::request<TBody, http::basic_fields<TAllocator>>;
	using TBody = http::string_body;
	using TAllocator = std::allocator<char>;

	struct HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( json&& j, HttpRequest&& req )ι:Json( move(j) ), Request{ move(req) }{}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		json Json;
		optional<HttpRequest> Request;//why optional?
	};
/*
	struct HttpTask{
		struct promise_type{
			promise_type()ι{}

			α get_return_object()ι->HttpTask{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			Φ unhandled_exception()ι->void;

			α MoveResult()ι->HttpTaskResult{ ASSERT(_result.Request); return move(_result); }//std::make_unique<HttpTaskResult>( move(x) ); }
			α SetRequest( HttpRequest&& req )ι->void{ ASSERT(!_result.Request); _result.Request.emplace( move(req) ); }//std::make_unique<HttpTaskResult>( move(x) ); }
			α SetResult( json&& j )ι->void{ ASSERT(_result.Request); _result.Json = move(j); }//std::make_unique<HttpTaskResult>( move(x) ); }
			template<http::status T> α SetException( RestException<T>&& e )ι->void{ _pException = mu<RestException<T>>(move(e)); }
			α TestException()ε->void{ if( _pException ) _pException->Throw(); }
		private:
			HttpTaskResult _result;
			up<IRestException> _pException;
		};
	};
	using HttpCo = coroutine_handle<HttpTask::promise_type>;
*/
	struct IHttpRequestAwait : TAwait<HttpTaskResult> {
		using base = TAwait<HttpTaskResult>;
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:base{sl},_request{ move(req) }{}
		virtual ~IHttpRequestAwait()=0;
		//α await_resume()ε->HttpTaskResult;
	protected:
		HttpRequest _request;
		up<json> _readyResult;
	};
	inline IHttpRequestAwait::~IHttpRequestAwait(){}
}
#undef Φ