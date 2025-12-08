#pragma once
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	struct SetMonitoringModeAwait final : TAwait<sp<UA_SetMonitoringModeResponse>>{
		using base=TAwait<sp<UA_SetMonitoringModeResponse>>;
		SetMonitoringModeAwait( sp<UAClient>&& c, uint32 subscriptionId, SRCE )ι:base{sl}, _client{move(c)}, _subscriptionId{subscriptionId}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α OnComplete( UA_SetMonitoringModeResponse& response )ι->void;
		α await_resume()ι->sp<UA_SetMonitoringModeResponse> override;//{ return _pPromise->get_return_object().Result(); }

		Ω Resume( sp<UAClient> client, function<void(base::Handle&&)> resume )ι->void;
		Ω Resume( sp<UAClient> client )ι->void;
		Ω Resume( StatusCode sc, sp<UAClient> client )ι->void;
	private:
		sp<UAClient> _client;
		RequestId _requestId{};
		uint32 _subscriptionId;
	};
}