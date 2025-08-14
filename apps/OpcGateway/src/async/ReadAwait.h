#pragma once

namespace Jde::Opc{ struct Value; }
namespace Jde::Opc::Gateway {
	struct UAClient;
	struct ΓOPC ReadAwait final : TAwait<flat_map<NodeId, Value>>{
		ReadAwait( flat_set<NodeId> x, sp<UAClient> c, SRCE )ι;
		α Suspend()ι->void override;
	private:
		flat_set<NodeId> _nodes;
		sp<UAClient> _client;
	};
	namespace Read{
		α OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *value )ι->void;
	}
}