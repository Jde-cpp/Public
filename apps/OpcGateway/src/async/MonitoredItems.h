#pragma once

namespace Jde::Opc::Gateway{
	struct CreateMonitoredItemsRequest;
	struct UAClient;

	struct DeleteMonitoredItemsAwait : VoidAwait{
		DeleteMonitoredItemsAwait( flat_map<SubscriptionId,flat_set<MonitorId>> monitoredItems, sp<UAClient> client, SRCE )ι;
		α await_ready()ι->bool override{ return !_client; }
		α Suspend()ι->void override;
		α OnComplete( UA_DeleteMonitoredItemsResponse& response, RequestId requestId )ι->void;
	private:
		sp<UAClient> _client;
		flat_map<SubscriptionId,flat_set<MonitorId>> _monitoredItems;
		flat_set<RequestId> _finished;
		constexpr static EOpcLogTags _tags{ EOpcLogTags::Monitoring };
	};
}