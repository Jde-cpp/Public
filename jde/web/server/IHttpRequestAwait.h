#pragma once
#include "usings.h"
#include "HttpRequest.h"
#include "RestException.h"

namespace Jde::Web::Server{
	struct ΓWS HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( json&& j, HttpRequest&& req )ι:Json( move(j) ), Request{ move(req) }{}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		json Json;
		optional<HttpRequest> Request; //why optional?
	};

	struct IHttpRequestAwait : TAwait<HttpTaskResult> {
		using base = TAwait<HttpTaskResult>;
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:base{sl},_request{ move(req) }{}
		virtual ~IHttpRequestAwait()=0;
	protected:
		HttpRequest _request;
		up<json> _readyResult;
	};
	inline IHttpRequestAwait::~IHttpRequestAwait(){}
}