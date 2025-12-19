#pragma once
#include <jde/ql/types/RequestQL.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct GatewayQLAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		GatewayQLAwait( QL::RequestQL&& q, sp<Web::Server::SessionInfo> session, bool returnRaw, SRCE )ι;
		α Suspend()ι->void;
	private:
		α GetClients()ι->TAwait<sp<UAClient>>::Task;
		α Query()ι->TAwait<jvalue>::Task;
		α Mutate()ι->TAwait<jvalue>::Task;
		α Introspect( QL::TableQL&& q )ι->jvalue;
		α ConnectionAttributes( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α ServerDescription( QL::TableQL&& q, sp<UAClient> client )ι->jobject;
		α SecurityPolicyUri( sp<UAClient> client )ι->jvalue;
		α SecurityMode( sp<UAClient> client )ι->jvalue;
		α OpcClientNK( const QL::TableQL& q )Ι->optional<ServerCnnctnNK>;
		α NeedsClient( const QL::TableQL& q )Ι->bool;

		bool _raw;
		QL::RequestQL _queries;
		flat_map<string,sp<UAClient>> _clients;
		sp<Web::Server::SessionInfo> _session;
	};
}