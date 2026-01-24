#pragma once
#include <jde/ql/types/RequestQL.h>
#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct GatewayQLAwait final : Web::Server::IQLAwait{
		using base = Web::Server::IQLAwait;
		GatewayQLAwait( QL::RequestQL&& q, variant<sp<Web::Server::SessionInfo>, Jde::UserPK> creds, bool returnRaw, SRCE )ι;
		α Suspend()ι->void;
	private:
		α GetClients()ι->TAwait<sp<UAClient>>::Task;
		α Query()ι->TAwait<jvalue>::Task;
		α ConnectionAttributes( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α ServerDescription( QL::TableQL&& q, sp<UAClient> client )ι->jobject;
		α SecurityPolicyUri( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α SecurityMode( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;

		flat_map<string,sp<UAClient>> _clients;
	};
}