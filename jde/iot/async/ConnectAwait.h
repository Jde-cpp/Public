#pragma once
#include "../Exports.h"

namespace Jde::Iot{
	struct UAClient; struct UAException;
	
	struct ΓI ConnectAwait final : IAwait{
		ConnectAwait( string&& id, SRCE )ι:IAwait{sl},_id{move(id)}{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override{ return _pPromise ? _pPromise->MoveResult() : _result; }
		Ω Resume( sp<UAClient>&& pClient, str target )ι->void;
		Ω Resume( sp<UAClient>&& pClient, str target, const UAException&& e )ι->void;
	private:
		Ω Resume( sp<UAClient> pClient, str target, function<void(HCoroutine&&)> resume )ι->void;
		α Create( string id )ι->Task;
		string _id;
		AwaitResult _result;
	};
}