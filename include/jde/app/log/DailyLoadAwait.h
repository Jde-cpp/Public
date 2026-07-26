#pragma once
#include <jde/fwk/co/LockKey.h>
#include <jde/app/proto/Log.pb.h>

namespace Jde::App{
	struct DailyLoadAwait : TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<CoLockGuard>::Task>{
		using base=TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<CoLockGuard>::Task>;
		//locked: the caller already holds the file's LockKey and keeps holding it past the read - ArchiveAwait must, to cover the read→archive→remove window.  Taking it again here would deadlock; LockKeyAwait is not reentrant.
		DailyLoadAwait( fs::path file, bool locked=false, SRCE )ι:base{sl},_file{ move(file) },_locked{locked}{}
		α Execute()ι->TAwait<CoLockGuard>::Task;
		α Read( optional<CoLockGuard> )ι->TAwait<string>::Task;
	private:
		fs::path _file;
		bool _locked;
	};
}