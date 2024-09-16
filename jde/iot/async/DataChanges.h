#pragma once
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot{
	//struct SocketSession;
	struct ΓI DatachangeAwait final : IAwait
	{
		DatachangeAwait( flat_set<NodeId>&& nodes, sp<IDataChange>&& dataChange, sp<UAClient>&& c, SRCE )ι:IAwait{sl}, _nodes{move(nodes)}, _dataChange{move(dataChange)}, _client{move(c)}{}
		α Suspend()ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		flat_set<NodeId> _nodes;
		sp<IDataChange> _dataChange;
		sp<UAClient> _client;
		Handle _requestId{};
		flat_map<NodeId, MonitoredItemCreateResult> _existingNodes;
	};

	Ξ DataChangesSubscribe( flat_set<NodeId> nodes, sp<IDataChange> socketSession, sp<UAClient> c )ι{ return DatachangeAwait{ move(nodes), move(socketSession), move(c) }; }
	α DataChangesDeleteCallback( UA_Client* ua, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext )->void;
	α DataChangesCallback( UA_Client *client, UA_UInt32 subId, void* subContext, UA_UInt32 monId, void* monContext, UA_DataValue *value )->void;
	α CreateDataChangesCallback(UA_Client* ua, void *userdata, RequestId requestId, void *response)ι->void;
	α MonitoredItemsDeleteCallback(UA_Client* ua, void *userdata, RequestId requestId, void *response)ι->void;

	using CreateMonitoredItemsResponsePtr = UAUP<UA_CreateMonitoredItemsResponse, UA_CreateMonitoredItemsResponse_delete>;
}