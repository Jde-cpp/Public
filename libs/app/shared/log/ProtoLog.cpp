#include <jde/app/log/ProtoLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/co/LockKey.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/app/proto/app.FromClient.h>
#include <jde/app/proto/LogProto.h>
#include "ArchiveAwait.h"

#define let const auto

namespace Jde::App{
	using Protobuf::ToGuid;
	//The day the daily file's *content* belongs to - not today's.  Seeding from now() meant a service that starts and stops
	//within one day never saw a day change, so _needsArchive was never set and log.binpb accumulated across every restart
	//cycle forever, with every logs() query deserializing the whole thing.  Only a process that happened to stay up across
	//midnight ever archived.  Seeded from the file instead, so the first entry of a new day arms the round that the previous
	//run should have done.
	//Last-write time rather than the first entry's timestamp: the file is appended on every flush, and reading it here would
	//deserialize the entire accumulated file on the startup path - the very cost this is fixing.  Residual window: a file
	//last appended just after midnight but holding only the previous day's entries reads as current, so its round waits for
	//the next day change.  Late, never lost - ArchiveAwait buckets entries by their own day, whenever the round runs.
	Ω dailyFileDay( const fs::path& dailyFile, const std::chrono::time_zone& tz )ι->std::chrono::year_month_day{
		std::error_code ec;
		let written = fs::last_write_time( dailyFile, ec );//ec overload: no file (the normal first-run case) is not an error here.
		//ToClock, because file_clock has no portable conversion to Clock: libc++ offers to_sys, MSVC only to_utc (which needs
		//the leap-second table and throws), and clang has no std::chrono::clock_cast.  Offsetting by both clocks' current
		//readings is exact to the sampling gap - orders of magnitude finer than the day this resolves to.
		return Chrono::LocalYMD( ec ? Clock::now() : Chrono::ToClock<Clock, fs::file_time_type::clock>(written), tz );
	}
	ProtoLog::ProtoLog( const jobject& settings )ε:
		Logging::ILogger{ settings },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) },
		_root{ Json::FindString(settings, "path").value_or((Process::AppDataFolder()/"logs").string()) },
		_tz{ Json::FindTimeZone(settings, "timeZone", *std::chrono::current_zone()) },
		_today{ dailyFileDay(DailyFile(), _tz) }{//_root and _tz are declared before _today, so both are live here.
		if( fs::exists(DailyFile()) )
			_dailyFileStart = TimePoint::min();
		Executor();//locks up if starts in StartTimer.
		Execution::Run();
		Process::AddShutdownFunction( [](bool /*terminate*/, SL){	//member Shutdown gets called after timer thread shutdown.
			if( auto log = Logging::FindLogger<App::ProtoLog>(); log )
				log->StopTimer();
		});
		try{
			fs::create_directories( _root );
		}
		catch( std::filesystem::filesystem_error& e ){
			throw IO::IOException( move(e) );
		}
	}
	α ProtoLog::Init()ι->void{
		Logging::Add<ProtoLog>( "proto" );
	}

	α ProtoLog::Shutdown( bool terminate, SL )ι->void{
		if( terminate )
			return;
		//Every other writer of the daily file holds its LockKey; this one did not.  The stream is size-prefixed, so
		//appending into the middle of a flush's write leaves a file nothing can parse, and appending into one an archive
		//round is about to fs::remove loses the entries silently.
		//Try, never wait: loggers are destroyed *after* the executor is (Process::cleanup), so a flush or round suspended
		//mid-flight will never resume to release this key - blocking here would hang the process instead of losing a buffer.
		auto lock = TryLockKey( DailyFile().string() );
		uint dropped{};
		{
			lg _{ _mutex };
			if( _toSave.empty() )
				return;
			if( lock ){
				try{
					IO::SaveBinary<byte>( DailyFile(), _toSave, true );
				}
				catch( const runtime_error& )
				{}
				return;
			}
			dropped = _toSave.size();
		}
		ERR( "Shutdown could not take the daily file's lock - {} buffered bytes dropped rather than interleaved into it.", dropped );//outside _mutex: this reaches the loggers that are still alive.
	}
	α ProtoLog::Deserialize( sv bytes )ε->vector<App::Log::Proto::FileEntry>{
		return Protobuf::DeserializeVector<App::Log::Proto::FileEntry>( bytes );
	}

	α ProtoLog::Write( const Logging::Entry& e )ι->void{
		if( !empty(e.Tags & _tags) )//recursion guard
			return;
		auto proto = LogProto::LogEntryFile( e );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_entry() = move( proto );
		Write( e, move(fileEntry) );
	}

	α ProtoLog::Write( const Logging::Entry& e, App::ProgramPK appPK, App::ProgInstPK instancePK )ι->void{
		if( !appPK || !instancePK || (appPK==_appPK && instancePK==_instancePK) )
			return Write( e );
		if( !empty(e.Tags & _tags) )//recursion guard
			return;
		auto proto = LogProto::LogEntryFile( e, appPK, instancePK );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_external_entry() = move( proto );
		Write( e, move(fileEntry) );
	}
	α ProtoLog::Write( const Logging::Entry& e, App::Log::Proto::FileEntry&& fileEntry )ι->void{
		auto data = Protobuf::SizePrefixed( fileEntry );
		_mutex.lock();
		_dailyFileStart = std::min<TimePoint>( _dailyFileStart, e.Time );//inside the lock: read by the query coroutine, written by every logging thread.
		if( auto day = Chrono::LocalYMD(e.Time, _tz); _today!=day ){
			_today = day;
			_needsArchive = true;
		}
		AddString( e.Id(), e.Text );
		AddString( e.FileId(), e.File() );
		AddString( e.FunctionId(), e.Function() );
		switch( fileEntry.value_case() ){
			case App::Log::Proto::FileEntry::kEntry:
				AddArguments( e.Arguments, fileEntry.entry().args() );
			break;
			case App::Log::Proto::FileEntry::kExternalEntry:
				AddArguments( e.Arguments, fileEntry.external_entry().args() );
			break;
			default:
				ASSERTX( false );
		}

		std::copy( data.begin(), data.end(), std::back_inserter(_toSave) );
		if( _toSave.size()>=_delaySize )
			Save();
		else{
			if( !_timer )
				StartTimer();
			_mutex.unlock();
		}
	}
	α ProtoLog::Save()ι->TAwait<CoLockGuard>::Task{
		auto toSave = move( _toSave );
		ResetTimerUnlocked();//_mutex is held on entry and released below.
		_toSave.reserve( toSave.size() );
		_cache.Trim();
		_mutex.unlock();
		Save( move(toSave), co_await LockKeyAwait{DailyFile().string()} );
	}
	α ProtoLog::Save( vector<byte> toSave, CoLockGuard )ι->VoidAwait::Task{
		try{
			TRACE( "Saving {} bytes to {}", toSave.size(), DailyFile().string() );
			//NOT _dailyFileStart = max() here.  A flush moves entries from the buffer into the daily file - both of which are
			//"local" - so parking the bound at max() after every successful flush told LogAwait::ShouldReadLocal that nothing
			//was local at all: `*_endTime > max` is false for any finite bound, so every time-bounded query silently skipped
			//the whole of today's log and answered out of the archives alone.  The bound is only released when the round
			//below actually takes the file away.
			co_await IO::WriteAwait( DailyFile(), vector<byte>{toSave}, true, IO::EWriteMode::Append, _tags );
		}
		catch( const runtime_error& ){
			lg _{ _mutex };
			_toSave.insert( _toSave.begin(), toSave.begin(), toSave.end() );//prepend: strings must precede the entries referencing them.
			co_return;
		}
		//An archive round reads the daily file only, so anything written while this flush was in flight has to land in the file
		//first or it is archived a round late.  It must not be archived out of the buffer instead: the round deletes the file but
		//cannot trim the buffer, so those entries would be archived again as soon as the next flush wrote them to disk.
		//Still holding the daily file's LockKey, so no other flush can interleave with the drain.
		vector<byte> pending;
		{
			lg _{ _mutex };
			if( !_needsArchive )
				co_return;
			pending = move( _toSave );
			_toSave.reserve( pending.size() );
			_needsArchive = false;//cleared here, under _mutex: a day-changed entry arriving after the drain re-arms it for the next round instead of being stranded in the daily file.
			_cache.Clear();
			_dailyFileStart = TimePoint::max();//_toSave is empty and everything local is bound for the file the round deletes; entries written from here lower it again.
		}
		if( pending.size() ){
			try{
				co_await IO::WriteAwait( DailyFile(), vector<byte>{pending}, true, IO::EWriteMode::Append, _tags );
			}
			catch( const runtime_error& ){
				lg _{ _mutex };
				_toSave.insert( _toSave.begin(), pending.begin(), pending.end() );
				_needsArchive = true;//nothing archived - re-arm so the next flush retries the round.
				_dailyFileStart = TimePoint::min();//the drain released the bound on the promise of a round that is not happening; those entries are local again, earliest unknown.
				co_return;
			}
		}
		Archive();
	}
	α ProtoLog::Archive()ι->VoidAwait::Task{
		try{
			co_await ArchiveAwait{ DailyFile(), _root, _tz };
		}
		catch( const runtime_error& ){
			lg _{ _mutex };//the round failed, so the daily file survives with everything the drain wrote into it - take the bound back.
			_dailyFileStart = TimePoint::min();
		}
	}

	α ProtoLog::AddString( uuid id, sv str )ι->void{
		AddString( id, str, _cache.Strings );
	}
	α ProtoLog::AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void{
		ASSERTX( str.size() || id==EmptyStringMd5 );
		if( find(cache, id)!=cache.end() )
			return;//TODO update position
		cache.push_front( id );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_str() = LogProto::ToString( id, string{str} );
		auto data = Protobuf::SizePrefixed( fileEntry );
		std::copy( data.begin(), data.end(), std::back_inserter(_toSave) );//TODO copy in SizePrefixed
	}
	α ProtoLog::AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void{
		ASSERTX( args.size()==(uint)ids.size() );
		for( uint i=0; i<args.size(); ++i )
			AddString( ToGuid(ids.Get((int)i)), args[i], _cache.Args );
	}

	α ProtoLog::StartTimer()ι->TimerAwait::Task{
		if( _delay==Duration::min() )
			co_return;
		_timer = mu<DurationTimer>( _delay, SRCE_CUR );
		let finished = co_await *_timer;
		if( finished ){
			_mutex.lock();
			_timer = nullptr;//let Write restart the timer for subsequent entries.
			if( !_toSave.empty() )
				Save();//unlocks _mutex.
			else
				_mutex.unlock();
		}
		else{
			lg _{ _mutex };
			if( _toSave.size() )
				StartTimer();
			else
				_timer = nullptr;
		}
	}

	//_mutex must be held.  StartTimer's continuation nulls _timer - destroying the DurationTimer - under this same lock, so
	//an unguarded `if( _timer ) _timer->Cancel()` could pass the test on the shutdown thread and then call Cancel() on
	//freed memory once the io thread ran the continuation.  Every other _timer access was already guarded; this one was
	//not, only because Save() calls it with the lock already held.
	//Safe to hold _mutex across Cancel(): it takes the DurationTimer's own lock and calls asio's cancel(), which *posts*
	//the completion - the continuation that re-takes _mutex never runs on this stack, so there is no re-entry to deadlock
	//on, and the lock order is only ever ProtoLog -> DurationTimer.
	α ProtoLog::ResetTimerUnlocked()ι->void{
		if( _timer )
			_timer->Cancel();
	}
	//One locked operation, because it is two writes: _delay is read by StartTimer under _mutex, so setting it from the
	//shutdown thread without the lock raced the very timer being cancelled.  min() is the sentinel StartTimer bails on.
	α ProtoLog::StopTimer()ι->void{
		lg _{ _mutex };
		_delay = Duration::min();
		ResetTimerUnlocked();
	}
	α ProtoLogCache::Clear()ι->void{
		Args.clear();
		Strings.clear();
	}
	α ProtoLogCache::Trim()ι->void{
		while( Args.size()>1000 )
			Args.pop_back();
		while( Strings.size()>1000 )
			Strings.pop_back();
	}
}