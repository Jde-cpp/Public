#pragma once
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot
{
	struct SetMonitoringModeAwait final : IAwait
	{
		SetMonitoringModeAwait( sp<UAClient>&& c, uint32 subscriptionId, SRCE )ι:IAwait{sl}, _client{move(c)}, _subscriptionId{subscriptionId}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;//{ return _pPromise->get_return_object().Result(); }

		Ω Resume( sp<UAClient> pClient, function<void(HCoroutine&&)> resume )ι->void;
		Ω Resume( sp<UAClient> pClient )ι->void;
		Ω Resume( StatusCode sc, sp<UAClient>&& pClient )ι->void;
	private:
		sp<UAClient> _client;
		uint32 _subscriptionId;
	};
	Ξ SetMonitoringMode( sp<UAClient>&& c, uint32 subscriptionId )ι->SetMonitoringModeAwait{ return SetMonitoringModeAwait{ move(c), subscriptionId }; }
	α SetMonitoringModeCallback( UA_Client *client, void *userdata, RequestId requestId, void *response )ι->void;
}