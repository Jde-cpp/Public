#pragma once
#include <jde/framework/coroutine/Await.h>
#include "../types/ServerCnnctn.h"
#include "../auth/OpcServerSession.h"

namespace Jde::Opc::Gateway{
	struct UAClient; struct UAClientException;

	struct ConnectAwait final : TAwait<sp<UAClient>>{
		using base = TAwait<sp<UAClient>>;
		ConnectAwait( string&& opcTarget, Credential cred, SRCE )ι:base{sl},_opcTarget{move(opcTarget)}, _cred{move(cred)}{}
		α Suspend()ι->void override;
		α await_resume()ε->sp<UAClient> override{ return Promise() ? base::await_resume() : _result; }
		Ω Resume( sp<UAClient> client )ι->void;
		Ω Resume( str target, Credential cred, const UAClientException&& e )ι->void;
	private:
		Ω Resume( str target, Credential cred, function<void(ConnectAwait::Handle)> resume )ι->void;
		α Create()ι->TAwait<vector<ServerCnnctn>>::Task;
		Ω EraseRequests( str opcNK, Credential cred, lg& _ )ι->vector<ConnectAwait::Handle>;
		string _opcTarget;
		Credential _cred;
		sp<UAClient> _result;
	};
}