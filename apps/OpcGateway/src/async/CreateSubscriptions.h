#pragma once

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct CreateSubscriptionAwait final : VoidAwait{
		CreateSubscriptionAwait( sp<UAClient> c, SRCE )ι:VoidAwait{sl}, _client{move(c)}{}
		α await_ready()ι->bool override;
		α Suspend()ι->void override;
	private:
		sp<UAClient> _client;
		RequestId _requestId{};
	};
}