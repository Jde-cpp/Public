#pragma once
#include <chrono>
#include <jde/fwk/settings.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/log/ILogger.h>
#include <jde/app/shared/proto/Log.pb.h>

namespace Jde::App{
	struct ProtoLogCache{
		α Trim()ι->void;
		std::deque<uuid> Args;
		std::deque<uuid> Strings;
	};
	struct ProtoLog final : Logging::ILogger, boost::noncopyable{
		ProtoLog()ε:ProtoLog( jobject{Settings::FindDefaultObject("/logging/proto")} ){}
		ProtoLog( const jobject& settings )ε;
		Ω Init()ι->void;

		α Shutdown( bool terminate )ι->void override;
		α DailyFile()ι->fs::path{ return _root/"log.binpb"; }
		α DailyFileStart()ι->TimePoint{ return _dailyFileStart; }//max if not started
		Ω Deserialize( sv bytes )ε->vector<App::Log::Proto::FileEntry>;
		α Entries()Ε->vector<App::Log::Proto::FileEntry>{ lg _{_mutex}; return Deserialize( sv{(char*)_toSave.data(), _toSave.size()} ); }
		α Name()Ι->string override{ return "ProtoLog"; }
		α Root()Ι->const fs::path&{ return _root; }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α TimeZone()Ι->const std::chrono::time_zone&{ return _tz; }
		α Write( const Logging::Entry& m )ι->void override;
	private:
		α AddString( uuid id, sv str )ι->void;
		α AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void;
		α AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void;
		α Save()ι->TAwait<CoLockGuard>::Task;
		α Save( vector<byte> toSave, CoLockGuard l )ι->VoidAwait::Task;

		α StartTimer()ι->VoidAwait::Task;
		α ResetTimer()ι->void;

		ProtoLogCache _cache;
		TimePoint _dailyFileStart;
		Duration _delay;
		const uint16 _delaySize{8096};
		mutable mutex _mutex;
		bool _needsArchive{false};
		fs::path _root;
		static constexpr ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::IO };
		up<DurationTimer> _timer;
		const std::chrono::time_zone& _tz;
		std::chrono::year_month_day _today;
		vector<byte> _toSave;
		flat_set<std::chrono::year_month_day> _archivedDays;
	};
}
