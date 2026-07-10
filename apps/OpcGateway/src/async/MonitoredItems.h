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
		α SuspendOnStrand()ι->void;//the UA submissions, dispatched onto the client's strand by Suspend.
		α TryResume()ι->void;//resume once all successfully-submitted deletes have completed.
		sp<UAClient> _client;
		flat_map<SubscriptionId,flat_set<MonitorId>> _monitoredItems;
		std::mutex _mutex;
		flat_set<RequestId> _finished;
		uint _submitted{};//count of async deletes actually accepted (not _monitoredItems.size(): a mid-loop submit failure means fewer callbacks will fire).
		bool _submissionsComplete{};
		bool _resumed{};
		constexpr static EOpcLogTags _tags{ EOpcLogTags::Monitoring };
	};
}