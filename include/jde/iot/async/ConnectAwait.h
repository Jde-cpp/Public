#pragma once
#include "../Exports.h"

namespace Jde::Iot{
	struct UAClient; struct UAException;
	
	struct ΓI ConnectAwait final : IAwait{
		ConnectAwait( string&& id, string&& userId, string&& pw, SRCE )ι:IAwait{sl},_id{move(id)}, _userId{move(userId)}, _password{move(pw)}{}
		α Suspend()ι->void override;
		α await_resume()ι->AwaitResult override{ return _pPromise ? _pPromise->MoveResult() : _result; }
		Ω Resume( sp<UAClient>&& pClient, str target, str userId )ι->void;
		Ω Resume( sp<UAClient>&& pClient, str target, str userId, const UAException&& e )ι->void;
	private:
		Ω Resume( sp<UAClient> pClient, str target, str userId, function<void(HCoroutine&&)> resume )ι->void;
		α Create( string opcServerId, string userId, string password )ι->Task;
		string _id;
		string _userId;
		string _password;
		AwaitResult _result;
	};
}