#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/types/TableQL.h>
#include <jde/app/log/ArchiveFile.h>
#include <jde/app/log/ProtoLog.h>

namespace Jde::App{
	struct LogAwait final : TAwaitEx<ArchiveFile,TAwait<vector<App::Log::Proto::FileEntry>>::Task>{
		using base = TAwaitEx<ArchiveFile,TAwait<vector<App::Log::Proto::FileEntry>>::Task>;
		LogAwait( QL::TableQL ql, SRCE )ι:base{sl}, _ql{move(ql)}{}
		α Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task;
		α ReadArchive( optional<TimePoint> startTime, optional<TimePoint> endTime, vector<App::Log::Proto::FileEntry> entries )ι->TAwait<ArchiveFile>::Task;
	private:
		QL::TableQL _ql;
	};
}