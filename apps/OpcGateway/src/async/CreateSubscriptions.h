#pragma once

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct CreateSubscriptionAwait final : TAwait<sp<UA_CreateSubscriptionResponse>>{
		using base = TAwait<sp<UA_CreateSubscriptionResponse>>;
		CreateSubscriptionAwait( sp<UAClient> c, SRCE )ι:base{sl}, _client{move(c)}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
		α await_resume()ε->sp<UA_CreateSubscriptionResponse> override;

		Ω Resume( sp<UAClient> pClient, function<void(CreateSubscriptionAwait::Handle)> resume )ι->void;
		Ω Resume( sp<UAClient> pClient )ι->void;
		Ω Resume( UAException&& e, sp<UAClient>&& pClient )ι->void;
	private:
		sp<UAClient> _client;
	};
	α StatusChangeNotificationCallback(UA_Client* ua, UA_UInt32 subId, void *subContext, UA_StatusChangeNotification *notification)ι->void;
	α DeleteSubscriptionCallback(UA_Client* ua, UA_UInt32 subId, void *subContext)ι->void;
	α CreateSubscriptionCallback(UA_Client* ua, void *userdata, RequestId requestId, UA_CreateSubscriptionResponse* response)ι->void;
}