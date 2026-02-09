#pragma once
#include <jde/ql/types/RequestQL.h>
#include "usings.h"
#include "HttpRequest.h"
#include "RestException.h"

namespace Jde::Web::Server{
	struct ΓWS HttpTaskResult{
		HttpTaskResult()=default;
		HttpTaskResult( HttpRequest&& req )ι:Request{move(req)}{}
		HttpTaskResult( HttpTaskResult&& rhs )ι;
		HttpTaskResult( jvalue&& j, HttpRequest&& req, SRCE )ι:Json( move(j) ), Request{ move(req) }, Source{sl}{}
		α operator=( HttpTaskResult&& rhs)ι->HttpTaskResult&;

		jvalue Json;
		optional<HttpRequest> Request; //why optional?
		optional<SL> Source;
	};

	struct IHttpRequestAwait : TAwait<HttpTaskResult> {
		using base = TAwait<HttpTaskResult>;
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:base{sl},_request{ move(req) }{}
		virtual ~IHttpRequestAwait()=0;
	protected:
		HttpRequest _request;
		up<jvalue> _readyResult;
		α Query()ι->TAwait<jvalue>::Task;
	private:
		β Schemas()Ι->const vector<sp<DB::AppSchema>>& = 0;
	};
	inline IHttpRequestAwait::~IHttpRequestAwait(){}
}