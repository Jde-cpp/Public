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

	struct ΓWS HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( json&& j, HttpRequest&& req )ι:Json( move(j) ), Request{ move(req) }{}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		json Json;
		optional<HttpRequest> Request;//why optional?
	};

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