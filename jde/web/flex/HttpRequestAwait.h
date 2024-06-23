#pragma once
#include <jde/web/usings.h>
#include <jde/web/flex/Streams.h>
#include <jde/web/flex/HttpRequest.h>
#include <jde/web/flex/RestException2.h>

#define Φ Γ auto
namespace Jde::Web::Flex{
	using TRequestType = http::request<TBody, http::basic_fields<TAllocator>>;
	using TBody = http::string_body;
	using TAllocator = std::allocator<char>;
	//using HttpTaskResult = json;
struct HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( HttpRequest&& req, json&& j )ι:Request{ move(req) }, Json( move(j) ){}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		optional<HttpRequest> Request;
		json Json;
	};

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

//	template<class TBody, class TAllocator>
	struct IHttpRequestAwait{
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:_input{ move(req) }, _sl{sl}{}
		virtual ~IHttpRequestAwait()=0;
		β await_ready()ι->bool{ return false; }
		β await_suspend( HttpCo h )ε->void{ _pPromise = &h.promise(); /*_pPromise->SetRequest( move(*_input) );*/}; //derived can throw
		β await_resume()ε->HttpTaskResult;
	protected:
		HttpTask::promise_type* _pPromise{};
		optional<HttpRequest> _input;
		SL _sl;
	};
}
#undef Φ