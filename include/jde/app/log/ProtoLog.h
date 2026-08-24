#pragma once
#include <chrono>
#include <jde/fwk/settings.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/log/ILogger.h>
#include <jde/app/proto/Log.pb.h>
#include "../usings.h"

namespace Jde::App{
	//L2: was two deques scanned linearly - up to 1000 uuid comparisons, three-plus times per line, on the path of every Debug+ line
	//of every thread and under ProtoLog's one mutex.  Worse, a hit did not reorder (the standing `//TODO update position`), so the
	//hottest strings drifted to the back and Trim evicted precisely the ones about to be used again, which then had to be re-emitted.
	//Now a map keyed by id with the last-use sequence as its value: the lookup is a binary search, a hit is a store, and Trim keeps
	//the most recently used.
	struct ProtoLogCache{
		α Clear()ι->void;
		α Trim()ι->void;
		α Touch( flat_map<uuid,uint>& cache, uuid id )ι->bool;//true when the id was not cached - the caller emits the string record only then.
		flat_map<uuid,uint> Args;
		flat_map<uuid,uint> Strings;
		uint Sequence{};
	};
	struct ProtoLog final : Logging::ILogger, noncopyable{
		ProtoLog( const jobject& settings )ε;
		~ProtoLog();
		Ω Init()ι->void;

		α Archive()ι->VoidAwait::Task;
		α Shutdown( bool terminate, SL sl )ι->void override;
		α DailyFile()ι->fs::path{ return _root/"log.binpb"; }
		α DailyFileStart()Ι->TimePoint{ lg _{_mutex}; return _dailyFileStart; }
		Ω Deserialize( sv bytes )ε->vector<App::Log::Proto::FileEntry>;
		α Entries()Ε->vector<App::Log::Proto::FileEntry>;//the buffer *and* whatever is in flight - see the definition (L1).
		α BufferSize()Ι->uint{ lg _{_mutex}; return _toSave.size(); }//what M5's cap bounds - the unflushed buffer alone, not _inFlight.
		α Name()Ι->sv override{ return "ProtoLog"; }
		α Root()Ι->const fs::path&{ return _root; }
		α SetMinLevel( ELogLevel /*level*/ )ι->void override{}
		α SetAppPKs( App::ProgramPK appPK, App::ProgInstPK instancePK )ι->void{ _appPK = appPK; _instancePK = instancePK; }
		α TimeZone()Ι->const std::chrono::time_zone&{ return _tz; }
		α Today()Ι->std::chrono::year_month_day{ lg _{_mutex}; return _today; }
		α Write( const Logging::Entry& m )ι->void override;
		α Write( const Logging::Entry& m, App::ProgramPK appPK, App::ProgInstPK instancePK )ι->void override;
	private:
		App::ProgramPK _appPK{};
		App::ProgInstPK _instancePK{};
		α Write( const Logging::Entry& m, App::Log::Proto::FileEntry&& entry )ι->void;
		α AddString( uuid id, sv str )ι->void;
		α AddString( uuid id, sv str, flat_map<uuid,uint>& cache )ι->void;
		α AddArguments( const vector<string>& args, const ::google::protobuf::RepeatedPtrField<std::string>& ids )ι->void;//L2: by reference - it was copying the whole field per entry.
		α Save()ι->TAwait<CoLockGuard>::Task;
		α Save( vector<byte> toSave, uint flushId, CoLockGuard l )ι->VoidAwait::Task;
		α DropBufferUnlocked()ι->uint;//_mutex must already be held - see the definition.  Returns the bytes dropped, 0 while under the cap.

		α StartTimer()ι->TimerAwait::Task;
		α ResetTimerUnlocked()ι->void;//_mutex must already be held - see the definition.
		α StopTimer()ι->void;//shutdown: takes _mutex, and no timer starts again.

		ProtoLogCache _cache;
		TimePoint _dailyFileStart{ TimePoint::max() };//re-seeded in the ctor when a daily file already exists; guarded by _mutex.
		Duration _delay;
		const uint16 _delaySize{8096};
		bool _flushFailed{};//M5: a flush that failed suppresses the size trigger, so the timer retries instead of every log line starting a fresh Save.
		uint _droppedBytes{};//reported and reset when the file becomes writable again.
		const uint32 _maxBufferSize;
		mutable mutex _mutex;
		bool _needsArchive{false};
		atomic<uint> _running{};
		fs::path _root;
		static constexpr ELogTags _tags{ ELogTags::ExternalLogger };
		up<DurationTimer> _timer;
		const std::chrono::time_zone& _tz;
		std::chrono::year_month_day _today;
		vector<byte> _toSave;
		flat_map<uint,vector<byte>> _inFlight;
		uint _flushId{};
	};
}
