#pragma once
#include <chrono>
#include <jde/fwk/settings.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/log/ILogger.h>

namespace Jde::App{
	struct ProtoLogCache{
		α Trim()ι->void;
		std::deque<uuid> Args;
		std::deque<uuid> Strings;
	};
	struct ProtoLog final : Logging::ILogger, boost::noncopyable{
		struct LoadDailyAwait : TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<string>::Task>{
			using base=TAwaitEx<vector<App::Log::Proto::FileEntry>,TAwait<string>::Task>;
			LoadDailyAwait( fs::path file, SRCE )ι:base{sl},_file{ move(file) }{}
			α Execute()ι->TAwait<string>::Task;
		private:
			fs::path _file;
		};
		struct ArchiveAwait : VoidAwait{
			ArchiveAwait( fs::path dailyFile, fs::path path, const std::chrono::time_zone& tz, SRCE )ι:VoidAwait{sl}, _dailyFile{ move(dailyFile) }, _path{ move(path) }, _tz{tz} {}
			α Suspend()ι->void override{ Execute(); }
			α Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task;
		private:
			α Append( fs::path file, App::Log::Proto::ArchiveFile values )ι->TAwait<string>::Task;
			α Save( fs::path file, const App::Log::Proto::ArchiveFile values )ι->VoidAwait::Task;
			fs::path _dailyFile;
			fs::path _path;
			const std::chrono::time_zone& _tz;
		};

		ProtoLog()ε:ProtoLog( jobject{Settings::FindDefaultObject("/logging/proto")} ){}
		ProtoLog( const jobject& settings )ε;
		α Shutdown( bool terminate )ι->void override;
		α Write( const Logging::Entry& m )ι->void override;
		α Name()Ι->string override{ return "ProtoLog"; }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α Load()ι->ProtoLog::LoadDailyAwait{ return LoadDailyAwait{DailyFile()}; };

	private:
		α AddString( uuid id, sv str )ι->void;
		α AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void;
		α AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void;
		α Save()ι->TAwait<CoLockGuard>::Task;
		α Save( vector<byte> toSave, CoLockGuard l )ι->VoidAwait::Task;

		α StartTimer()ι->VoidAwait::Task;
		α ResetTimer()ι->void;
		α DailyFile()ι->fs::path{ return _path/"log.binpb"; }

		ProtoLogCache _cache;
		Duration _delay;
		const uint16 _delaySize{8096};
		mutex _mutex;
		bool _needsArchive{false};
		fs::path _path;
		static constexpr ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::IO };
		up<DurationTimer> _timer;
		const std::chrono::time_zone& _tz;
		std::chrono::year_month_day _today;
		vector<byte> _toSave;
		flat_set<std::chrono::year_month_day> _archivedDays;
	};
}
