#pragma once
#include <jde/ql/types/RequestQL.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct GatewayQLAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		GatewayQLAwait( QL::RequestQL&& q, sp<Web::Server::SessionInfo> session, bool returnRaw, SRCE )ι;
		GatewayQLAwait( QL::RequestQL&& q, UserPK executer, bool raw, SL sl )ι;
		α Suspend()ι->void;
	private:
		α GetClients()ι->TAwait<sp<UAClient>>::Task;
		α Query()ι->TAwait<jvalue>::Task;
		α Session()ι->sp<Web::Server::SessionInfo>{ return std::holds_alternative<sp<Web::Server::SessionInfo>>(_creds) ? get<sp<Web::Server::SessionInfo>>(_creds) : nullptr; }
		α UserPK()ι->Jde::UserPK{ return std::holds_alternative<Jde::UserPK>(_creds) ? get<Jde::UserPK>(_creds) : Session()->UserPK; }
		α Introspect( QL::TableQL&& q )ι->jvalue;
		α ConnectionAttributes( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α ServerDescription( QL::TableQL&& q, sp<UAClient> client )ι->jobject;
		α SecurityPolicyUri( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α SecurityMode( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;

		variant<sp<Web::Server::SessionInfo>, Jde::UserPK> _creds;
		flat_map<string,sp<UAClient>> _clients;
		QL::RequestQL _queries;
		bool _raw;
	};
}