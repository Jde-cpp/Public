#pragma once
#ifndef SESSION_AWAIT_H_
#define SESSION_AWAIT_H_
#include "../Exports.h"

namespace Jde::Iot{
	struct UAClient;
	struct ΓI SessionAwait final : IAwait{
		SessionAwait( sp<UAClient> client, SRCE )ι:IAwait{sl}, _client{move(client)}{}
		α Suspend()ι->void override;
		Ω Trigger( sp<UAClient>&& pClient )ι->void;
		α await_resume()ι->AwaitResult override{ return {}; }
	private:
		sp<UAClient> _client;
	};
	Ξ AwaitSessionActivation( sp<UAClient> p, SRCE )ι->SessionAwait{ return SessionAwait{p, sl}; }
}
#endif