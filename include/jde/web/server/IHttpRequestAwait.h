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

	struct IQLAwait : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		IQLAwait( QL::RequestQL&& q, variant<sp<SessionInfo>, Jde::UserPK> creds, bool returnRaw, SRCE )ι:
			base{sl}, _creds{move(creds)}, _queries{move(q)}, _raw{returnRaw}{}
		virtual ~IQLAwait()=0;
	protected:
		α Session()ι->sp<Web::Server::SessionInfo>{ return std::holds_alternative<sp<Web::Server::SessionInfo>>(_creds) ? get<sp<Web::Server::SessionInfo>>(_creds) : nullptr; }
		α UserPK()ι->Jde::UserPK{ return std::holds_alternative<Jde::UserPK>(_creds) ? get<Jde::UserPK>(_creds) : Session()->UserPK; }

		variant<sp<SessionInfo>, Jde::UserPK> _creds;
		QL::RequestQL _queries;
		bool _raw;
	};
	inline IQLAwait::~IQLAwait(){}

	struct IHttpRequestAwait : TAwait<HttpTaskResult> {
		using base = TAwait<HttpTaskResult>;
		IHttpRequestAwait( HttpRequest&& req, SRCE )ι:base{sl},_request{ move(req) }{}
		virtual ~IHttpRequestAwait()=0;
	protected:
		HttpRequest _request;
		up<jvalue> _readyResult;
		α Query()ι->TAwait<jvalue>::Task;
	private:
		β QueryHandler( QL::RequestQL&& q, variant<sp<SessionInfo>, Jde::UserPK> creds, bool returnRaw, SRCE )ι->up<IQLAwait> = 0;
		β Schemas()Ι->const vector<sp<DB::AppSchema>>& = 0;
	};
	inline IHttpRequestAwait::~IHttpRequestAwait(){}
}