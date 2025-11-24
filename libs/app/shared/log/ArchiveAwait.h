#pragma once
#include <jde/fwk/co/LockKey.h>
#include <jde/ql/types/TableQL.h>
#include <jde/app/log/ArchiveFile.h>
#include <jde/app/proto/Log.pb.h>

namespace Jde::App{
		struct ArchiveAwait : VoidAwait{
			ArchiveAwait( fs::path dailyFile, fs::path path, const std::chrono::time_zone& tz, SRCE )ι:VoidAwait{sl}, _dailyFile{ move(dailyFile) }, _path{ move(path) }, _tz{tz} {}
			α Suspend()ι->void override{ Execute(); }
			α Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task;
		private:
			α Append( fs::path file, App::Log::Proto::ArchiveFile values )ι->TAwait<string>::Task;
			α Save( flat_map<std::chrono::year_month_day, App::Log::Proto::ArchiveFile> archives )ι->VoidAwait::Task;
			α Save( fs::path file, const App::Log::Proto::ArchiveFile values )ι->VoidAwait::Task;

			fs::path _dailyFile;
			fs::path _path;
			const std::chrono::time_zone& _tz;
		};

		struct ArchiveLoadAwait final : TAwaitEx<ArchiveFile,StringAwait::Task>{
			using base=TAwaitEx<ArchiveFile,StringAwait::Task>;
			ArchiveLoadAwait( optional<TimePoint> startTime, optional<TimePoint> endTime, QL::TableQL q, const std::chrono::time_zone& tz, const fs::path& root, SRCE )ι:base{sl}, _startTime{ move(startTime) }, _endTime{ move(endTime) }, _query{move(q)}, _root{root}, _tz{tz}{}
			α Execute()ι->StringAwait::Task;
		private:
			optional<TimePoint> _startTime;
			optional<TimePoint> _endTime;
			QL::TableQL _query;
			fs::path _root;
			const std::chrono::time_zone& _tz;
		};
	}