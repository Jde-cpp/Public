#pragma once

namespace Jde::Opc{ struct Value; }
namespace Jde::Opc::Gateway {
	struct UAClient;
	struct ReadValueAwait final : TAwait<flat_map<NodeId, Value>>{
		using base = TAwait<flat_map<NodeId, Value>>;
		ReadValueAwait( flat_set<NodeId> x, sp<UAClient> c, SRCE )ι;
		α await_ready()ι->bool override{ return _nodes.size()==0; }
		α Suspend()ι->void override;
		α OnComplete( RequestId requestId, StatusCode sc, UA_DataValue* val )ι->void;
	private:
		flat_set<NodeId> _nodes;
		sp<UAClient> _client;
		flat_map<RequestId, NodeId> _requests;
		flat_map<NodeId, Value> _results;
	};
}