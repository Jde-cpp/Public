#pragma once
#include <jde/ql/types/RequestQL.h>

namespace Jde::Opc::Gateway{
	struct GatewayQLAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		GatewayQLAwait( QL::RequestQL&& q, sp<Web::Server::SessionInfo> session, bool returnRaw, SRCE )ι;
		α Suspend()ι->void;
	private:
		α Query()ι->TAwait<jvalue>::Task;
		α Mutate()ι->TAwait<jvalue>::Task;
		α Introspect( QL::TableQL&& q )ι->jvalue;

		bool _raw;
		QL::RequestQL _queries;
		sp<Web::Server::SessionInfo> _session;
	};
}