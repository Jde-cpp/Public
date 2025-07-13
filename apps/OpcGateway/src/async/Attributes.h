#pragma once
//#include "../UAClient.h"

namespace Jde::Opc{ struct ExNodeId; }

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct AttribAwait final : TAwait<flat_map<ExNodeId, ExNodeId>>{
		using base = TAwait<flat_map<ExNodeId, ExNodeId>>;
		AttribAwait( flat_set<ExNodeId>&& x, sp<UAClient>&& c, SRCE )ι:base{sl}, _values{move(x)},_client{move(c)}{}
		α Suspend()ι->void override;
	private:
		flat_set<ExNodeId> _values;
		sp<UAClient> _client;
	};
namespace Attributes{
	α OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_NodeId *dataType )ι->void;
}}