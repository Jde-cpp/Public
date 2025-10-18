#pragma once
#include <jde/ql/types/RequestQL.h>
#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::Opc::Gateway{
	struct GatewayQLAwait final : TAwait<Web::Server::HttpTaskResult>{
		using base = TAwait<Web::Server::HttpTaskResult>;
		GatewayQLAwait( Web::Server::HttpRequest&& request, QL::RequestQL&& q, SRCE )ι;
		α Suspend()ι->void;
	private:
		α Query()ι->TAwait<jvalue>::Task;
		α Mutate()ι->TAwait<jvalue>::Task;
		α Introspect( QL::TableQL&& q )ι->jvalue;
		bool _raw;
		Web::Server::HttpRequest _request;
		QL::RequestQL _queries;
	};
}