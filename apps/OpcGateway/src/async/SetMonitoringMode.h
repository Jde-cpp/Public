#pragma once
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	struct SetMonitoringModeAwait final : TAwait<sp<UA_SetMonitoringModeResponse>>{
		using base=TAwait<sp<UA_SetMonitoringModeResponse>>;
		SetMonitoringModeAwait( sp<UAClient>&& c, uint32 subscriptionId, SRCE )ι:base{sl}, _client{move(c)}, _subscriptionId{subscriptionId}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->sp<UA_SetMonitoringModeResponse> override;//{ return _pPromise->get_return_object().Result(); }

		Ω Resume( sp<UAClient> pClient, function<void(base::Handle&&)> resume )ι->void;
		Ω Resume( sp<UAClient> pClient )ι->void;
		Ω Resume( StatusCode sc, sp<UAClient>&& pClient )ι->void;
	private:
		sp<UAClient> _client;
		uint32 _subscriptionId;
	};
	α SetMonitoringModeCallback( UA_Client *client, void *userdata, RequestId requestId, UA_SetMonitoringModeResponse* response )ι->void;
}