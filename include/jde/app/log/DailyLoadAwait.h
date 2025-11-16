#pragma once
#include <jde/fwk/co/LockKey.h>
#include <jde/app/proto/Log.pb.h>

namespace Jde::App{
	struct DailyLoadAwait : TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<CoLockGuard>::Task>{
		using base=TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<CoLockGuard>::Task>;
		DailyLoadAwait( fs::path file, SRCE )ι:base{sl},_file{ move(file) }{}
		α Execute()ι->TAwait<CoLockGuard>::Task;
		α Read( CoLockGuard )ι->TAwait<string>::Task;
	private:
		fs::path _file;
	};
}