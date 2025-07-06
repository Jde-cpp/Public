#pragma once
#ifndef SESSION_AWAIT_H_
#define SESSION_AWAIT_H_
#include "../exports.h"
#include <jde/framework/coroutine/Await.h>

namespace Jde::Opc{
	struct UAClient;
	struct ΓOPC SessionAwait final : VoidAwait<>{
		SessionAwait( sp<UAClient> client, SRCE )ι:VoidAwait{sl}, _client{move(client)}{}
		α Suspend()ι->void override;
		Ω Trigger( sp<UAClient>&& pClient )ι->void;
	private:
		sp<UAClient> _client;
	};
	Ξ AwaitSessionActivation( sp<UAClient> p, SRCE )ι->SessionAwait{ return SessionAwait{p, sl}; }
}
#endif