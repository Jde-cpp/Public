#pragma once
//#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	struct IDataChange; struct UAClient; struct MonitoredItemCreateResult;
	struct DataChangeAwait final : TAwait<FromServer::SubscriptionAck>{
		using base = TAwait<FromServer::SubscriptionAck>;
		DataChangeAwait( flat_set<NodeId> nodes, sp<IDataChange> dataChange, sp<UAClient> c, SRCE )ι:base{sl}, _nodes{move(nodes)}, _dataChange{move(dataChange)}, _client{move(c)}{}
		α Suspend()ι->void override;
		α await_resume()ι->FromServer::SubscriptionAck override;
		α OnComplete( UA_CreateMonitoredItemsResponse* response )ι->void;
	private:
		flat_set<NodeId> _nodes;
		sp<IDataChange> _dataChange;
		sp<UAClient> _client;
		RequestId _requestId{};
		Jde::Handle _monitoredRequestId{};
		flat_map<NodeId, MonitoredItemCreateResult> _existingNodes;
	};
}