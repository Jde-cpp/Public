#pragma once
#include "../exports.h"
#include <jde/framework/coroutine/Await.h>
#include "../types/OpcClient.h"

namespace Jde::Opc{
	struct UAClient; struct UAException;

	struct ΓOPC ConnectAwait final : TAwait<sp<UAClient>>{
		using base = TAwait<sp<UAClient>>;
		ConnectAwait( string&& opcTarget, string&& loginName, string&& pw, SRCE )ι:base{sl},_opcTarget{move(opcTarget)}, _loginName{move(loginName)}, _password{move(pw)}{}
		α Suspend()ι->void override;
		α await_resume()ε->sp<UAClient> override{ return Promise() ? base::await_resume() : _result; }
		Ω Resume( sp<UAClient> pClient, str target, str loginName )ι->void;
		Ω Resume( sp<UAClient> pClient, str target, str loginName, const UAException&& e )ι->void;
	private:
		Ω Resume( sp<UAClient> pClient, str target, str loginName, function<void(ConnectAwait::Handle)> resume )ι->void;
		α Create()ι->OpcClientAwait::Task;
		string _opcTarget;
		string _loginName;
		string _password;
		sp<UAClient> _result;
	};
}