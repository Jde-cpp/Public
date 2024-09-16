#pragma once
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot{
	struct ΓI CreateSubscriptionAwait final : IAwait
	{
		CreateSubscriptionAwait( sp<UAClient>&& c, SRCE )ι:IAwait{sl}, _client{move(c)}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ι->AwaitResult override;

		Ω Resume( sp<UAClient> pClient, function<void(HCoroutine&&)> resume )ι->void;
		Ω Resume( sp<UAClient> pClient )ι->void;
		Ω Resume( UAException&& e, sp<UAClient>&& pClient )ι->void;
	private:
		sp<UAClient> _client;
	};
	Ξ CreateSubscription( sp<UAClient> c )ι->CreateSubscriptionAwait{ return CreateSubscriptionAwait{ move(c) }; }
	α StatusChangeNotificationCallback(UA_Client* ua, UA_UInt32 subId, void *subContext, UA_StatusChangeNotification *notification)ι->void;
	α DeleteSubscriptionCallback(UA_Client* ua, UA_UInt32 subId, void *subContext)ι->void;
	α CreateSubscriptionCallback(UA_Client* ua, void *userdata, RequestId requestId, void *response)ι->void;
}