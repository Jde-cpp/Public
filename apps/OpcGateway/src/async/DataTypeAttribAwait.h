#pragma once

namespace Jde::Opc{ struct NodeId; }
namespace Jde::Opc::Gateway{
	struct UAClient;
	struct DataTypeAttribAwait final : TAwait<flat_map<NodeId, variant<NodeId, StatusCode>>>{
		using base = TAwait<flat_map<NodeId, variant<NodeId, StatusCode>>>;
		DataTypeAttribAwait( flat_set<NodeId>&& x, sp<UAClient>&& c, SRCE )ι:base{ sl }, _nodeIds{ move(x) }, _client{ move(c) }{}
		α Suspend()ι->void override;
		α OnComplete( RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;
	private:
		flat_set<NodeId> _nodeIds;
		sp<UAClient> _client;
		flat_map<NodeId, variant<NodeId, StatusCode>> _results;
		flat_map<RequestId, NodeId> _requestNodes;
	};
}