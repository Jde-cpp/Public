#pragma once
#include "usings.h"
#include "HttpRequest.h"
#include "RestException.h"

namespace Jde::Web::Server{
	struct ΓWS HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( jobject&& j, HttpRequest&& req, SRCE )ι:Json( move(j) ), Request{ move(req) }, Source{sl}{}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		jobject Json;
		optional<HttpRequest> Request; //why optional?
		optional<SL> Source;
	};

	struct IHttpRequestAwait : TAwait<HttpTaskResult> {
		using base = TAwait<HttpTaskResult>;
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:base{sl},_request{ move(req) }{}
		virtual ~IHttpRequestAwait()=0;
	protected:
		HttpRequest _request;
		up<jobject> _readyResult;
	};
	inline IHttpRequestAwait::~IHttpRequestAwait(){}
}