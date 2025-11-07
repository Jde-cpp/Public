#include <jde/app/log/ProtoLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/proto.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include "ArchiveAwait.h"

#define let const auto

namespace Jde::App{
	using Jde::Proto::ToGuid;
	ProtoLog::ProtoLog( const jobject& settings )ε:
		Logging::ILogger{ settings },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) },
		_root{ Json::FindString(settings, "path").value_or((Process::AppDataFolder()/"logs").string()) },
		_tz{ Json::FindTimeZone(settings, "timeZone", *std::chrono::current_zone()) },
		_today{ Chrono::LocalYMD(Clock::now(), _tz) }{
		Executor();//locks up if starts in StartTimer.
		Execution::Run();
		Process::AddShutdownFunction( []( bool /*terminate*/ ){	//member Shutdown gets called after timer thread shutdown.
			auto log = Logging::GetLogger<App::ProtoLog>();
			if( log ){
				log->_delay = Duration::min();
				log->ResetTimer();
			}
		});
		try{
			fs::create_directories( _root );
		}
		catch( std::filesystem::filesystem_error& e ){
			throw IOException( move(e) );
		}
		for( auto yearDir : fs::directory_iterator(_root) ){
			auto iyear = fs::is_directory(yearDir) ? Str::TryTo<int>(yearDir.path().filename().string()) : std::nullopt;
			if( !iyear || *iyear<2025 )
				continue;
			for( auto monthDir : fs::directory_iterator(yearDir) ){
				auto imonth = fs::is_directory(monthDir) ? Str::TryTo<unsigned>(monthDir.path().filename().string()) : std::nullopt;
				if( !imonth || *imonth>12 )
					continue;
				for( auto dayDir : fs::directory_iterator(monthDir) ){
					using namespace std::chrono;
					auto iday = fs::is_directory(dayDir) ? Str::TryTo<unsigned>(dayDir.path().filename().string()) : std::nullopt;
					if( iday && *iday>31 )
						_archivedDays.insert( year_month_day{year{*iyear},month{*imonth},day{*iday}} );
				}
			}
		}
	}
	α ProtoLog::Init()ι->void{
		Logging::Add<ProtoLog>( "proto" );
	}

	α ProtoLog::Shutdown( bool terminate )ι->void{
		if( !terminate && !_toSave.empty() ){
			try{
				lg _{_mutex};
				IO::SaveBinary<byte>( DailyFile(), _toSave );
			}
			catch( exception& )
			{}
		}
	}
	α ProtoLog::Deserialize( sv bytes )ε->vector<App::Log::Proto::FileEntry>{
		return Jde::Proto::DeserializeVector<App::Log::Proto::FileEntry>( bytes );
	}

	α ProtoLog::Write( const Logging::Entry& e )ι->void{
		if( !empty(e.Tags & _tags) )//recursion guard
			return;
		auto proto = FromClient::LogEntryFile( e );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_entry() = move(proto);
		_dailyFileStart = std::min<TimePoint>( _dailyFileStart, e.Time );
		auto data = Jde::Proto::SizePrefixed( fileEntry );
		_mutex.lock();
		if( auto day = Chrono::LocalYMD(e.Time, _tz); _today!=day ){
			_today = day;
			_needsArchive = true;
		}
		AddString( e.Id(), e.Text );
		AddString( e.FileId(), e.File() );
		AddString( e.FunctionId(), e.Function() );
		AddArguments( e.Arguments, fileEntry.entry().args() );
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
		ResetTimer();
		_toSave.reserve( toSave.size() );
		_cache.Trim();
		_mutex.unlock();
		Save( move(toSave), co_await LockKeyAwait{DailyFile().string()} );
	}
	α ProtoLog::Save( vector<byte> toSave, CoLockGuard )ι->VoidAwait::Task{
		try{
			co_await IO::WriteAwait( DailyFile(), move(toSave), true, _tags );
			_dailyFileStart = TimePoint::max();
		}
		catch( exception& )	{
			lg _{_mutex };
			std::copy( toSave.begin(), toSave.end(), std::back_inserter(_toSave) );
			_mutex.unlock();
			co_return;
		}
		if( _needsArchive ){
			try{
				co_await ArchiveAwait{ DailyFile(), _root, _tz };
				_needsArchive = false;
			}
			catch( const exception& )
			{}
		}
	}

	α ProtoLog::AddString( uuid id, sv str )ι->void{
		AddString( id, str, _cache.Strings );
	}
	α ProtoLog::AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void{
		if( let i = find( cache, id ); i!=cache.end() )
			return;//TODO update position
		cache.push_front( id );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_str() = FromClient::ToString(id, string{str});
		auto data = Jde::Proto::SizePrefixed( fileEntry );
		std::copy( data.begin(), data.end(), std::back_inserter(_toSave) );//TODO copy in SizePrefixed
	}
	α ProtoLog::AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void{
		ASSERT( args.size()==(uint)ids.size() );
		for( uint i=0; i<args.size(); ++i )
			AddString( ToGuid(ids.Get((int)i)), args[i], _cache.Args );
	}

	α ProtoLog::StartTimer()ι->VoidAwait::Task{
		if( _delay==Duration::min() )
			co_return;
		_timer = mu<DurationTimer>( _delay, _tags, SRCE_CUR );
		try{
			co_await *_timer;
			_mutex.lock();
			if( !_toSave.empty() )
				Save();
		}
		catch( const IException& ){
			lg _{_mutex};
			if( _toSave.size() )
				StartTimer();
			else
				_timer = nullptr;
		}
	}

	α ProtoLog::ResetTimer()ι->void{
		if( _timer )
			_timer->Cancel();
	}
	α ProtoLogCache::Trim()ι->void{
		while( Args.size()>1000 )
			Args.pop_back();
		while( Strings.size()>1000 )
			Strings.pop_back();
	}
}