#pragma once
#include "jde/fwk/usings.h"
#include <jde/fwk/co/Await.h>
#include <jde/ql/types/TableQL.h>
#include <jde/app/log/ArchiveFile.h>
#include <jde/app/log/ProtoLog.h>

namespace Jde::App{
	struct LogAwait final : TAwait<ArchiveFile>{
		using base = TAwait<ArchiveFile>;
		LogAwait( QL::TableQL ql, SRCE )ι:base{sl}, _ql{move(ql)}{}
		α Suspend()ι->void override;
		α ReadArchive( vector<App::Log::Proto::FileEntry> entries )ι->TAwait<ArchiveFile>::Task;
		α ReadLocal( optional<ArchiveFile> archive )ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task;
	private:
		α ShouldReadLocal()Ι->bool{ return !_endTime || *_endTime > _dailyFileStart; }
		QL::TableQL _ql;
		TimePoint _dailyFileStart;
		optional<TimePoint> _startTime;
		optional<TimePoint> _endTime;
	};
}