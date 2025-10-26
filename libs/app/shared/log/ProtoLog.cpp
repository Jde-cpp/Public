#include <jde/app/log/ProtoLog.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/proto.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/app/shared/proto/App.FromClient.h>

namespace Jde::App{
	#define let const auto
	using Jde::Proto::ToGuid;
	α ProtoLog::LoadDailyAwait::Execute()ι->TAwait<string>::Task{
		vector<App::Log::Proto::FileEntry> y;
		string content;
		try{
			content = co_await IO::ReadAwait( _file );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			co_return;
		}
		for( auto p = content.data(), end = content.data() + content.size(); p+4<end; ){
			uint32 length{};
			for( auto i=3; i>=0; --i ){
				const byte b = (byte)*p++;
				length = (length<<8) | (uint32)b;
			}
			if( p+length<content.data()+content.size() )
				y.push_back( Jde::Proto::Deserialize<App::Log::Proto::FileEntry>((google::protobuf::uint8*)p, (int)length) );
			p += length;
		}
		Resume( move(y) );
	}
	α ProtoLog::ArchiveAwait::Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task{
		vector<App::Log::Proto::FileEntry> entries;
		try{
			entries = co_await ProtoLog::LoadDailyAwait{ _dailyFile };
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			co_return;
		}
		using namespace std::chrono;
		flat_map<year_month_day, App::Log::Proto::ArchiveFile> archives;
		std::map<uuid,App::Log::Proto::String> strings;
		for( auto& entry : entries ){
			if( entry.has_str() )
				strings[ToGuid(entry.str().id())] = move( *entry.mutable_str() );
			else{
				let day = Chrono::LocalYMD( Jde::Proto::ToTimePoint(entry.entry().time()), _tz );
				*archives[day].add_entries() = move( *entry.mutable_entry() );
			}
		}
		for( auto& [ymd,archive] : archives ){
			for( int i=0; i<archive.entries_size(); ++i ){
				auto& entry = *archive.mutable_entries( i );
				if( auto p = strings.find(ToGuid(entry.template_id())); p!=strings.end() )
					*archive.add_templates() = p->second;
				if( auto p = strings.find(ToGuid(entry.file_id())); p!=strings.end() )
					*archive.add_files() = p->second;
				if( auto p = strings.find(ToGuid(entry.function_id())); p!=strings.end() )
					*archive.add_functions() = p->second;
				for( auto& argId : *entry.mutable_args() ){
					if( let i = strings.find(ToGuid(argId)); i!=strings.end() )
						*archive.add_args() = i->second;
				}
			}
		}
		for( auto& [ymd,archive] : archives ){
			let dir = _path/std::to_string((int)ymd.year())/std::to_string((unsigned)ymd.month())/std::to_string((unsigned)ymd.day());
			if( !fs::exists(dir) )
				fs::create_directories( dir );
			let file = dir/"archive.binpb";
			if( fs::exists(file) )
				Append( move(file), move(archive) );
			else
				Save( move(file), move(archive) );
		}
	}
	α ProtoLog::ArchiveAwait::Append( const fs::path archiveFile, App::Log::Proto::ArchiveFile append )ι->TAwait<string>::Task{
		string content;
		try{
			content = co_await IO::ReadAwait( archiveFile );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			co_return;
		}
		std::map<uuid,App::Log::Proto::String> args, templates, files, functions;
		std::map<TimePoint,App::Log::Proto::LogEntryFile> entries;
		auto existing = Jde::Proto::Deserialize<App::Log::Proto::ArchiveFile>( move(content) );
		auto addEntries = [&entries]( App::Log::Proto::ArchiveFile& af ){
			for( int i=0; i<af.entries_size(); ++i ){
				auto& entry = *af.mutable_entries(i);
				entries.emplace_hint( entries.end(), Jde::Proto::ToTimePoint(entry.time()), move(entry) );
			}
		};
		addEntries( existing );
		addEntries( append );
#define ADD_STRINGS( collection, af ) \
		for( int i=0; i<af.collection##_size(); ++i ){ \
			auto& s = *af.mutable_##collection(i); \
			collection[ToGuid(s.id())] = move(s); \
		}
		ADD_STRINGS( args, existing );
		ADD_STRINGS( args, append );
		ADD_STRINGS( templates, existing );
		ADD_STRINGS( templates, append );
		ADD_STRINGS( files, existing );
		ADD_STRINGS( files, append );
		ADD_STRINGS( functions, existing );
		ADD_STRINGS( functions, append );

		App::Log::Proto::ArchiveFile cumulative;
		for( auto& [_,entry] : entries )
			*cumulative.add_entries() = move(entry);
		for( auto& [_,s] : args )
			*cumulative.add_args() = move(s);
		for( auto& [_,s] : templates )
			*cumulative.add_templates() = move(s);
		for( auto& [_,s] : files )
			*cumulative.add_files() = move(s);
		for( auto& [_,s] : functions )
			*cumulative.add_functions() = move(s);

		Save( archiveFile, cumulative );
	}
	α ProtoLog::ArchiveAwait::Save( fs::path file, App::Log::Proto::ArchiveFile values )ι->VoidAwait::Task{
		try{
			co_await IO::WriteAwait( move(file), Jde::Proto::ToString(values), true, _tags );
			fs::remove( _dailyFile );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	ProtoLog::ProtoLog( const jobject& settings )ι:
		Logging::ILogger{ settings },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) },
		_path{ Json::FindString(settings, "path").value_or((Process::ApplicationDataFolder()/"logs").string()) },
		_tz{ Json::FindTimeZone(settings, "timeZone", *std::chrono::current_zone()) },
		_today{ Chrono::LocalYMD(Clock::now(), _tz) }{
		Executor();//locks up if starts in StartTimer.
		Execution::Run();
		Process::AddShutdownFunction( [this]( bool /*terminate*/ ){	//member Shutdown gets called after timer thread shutdown.
			_delay = Duration::min();
			ResetTimer();
		});
		for( auto yearDir : fs::directory_iterator(_path) ){
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

	α ProtoLog::Write( const Logging::Entry& e )ι->void{
		if( !empty(e.Tags & _tags) )//recursion guard
			return;
		auto proto = FromClient::LogEntryFile( e );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_entry() = move(proto);
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
		}
		catch( exception& )	{
			lg _{_mutex };
			std::copy( toSave.begin(), toSave.end(), std::back_inserter(_toSave) );
			_mutex.unlock();
			co_return;
		}
		if( _needsArchive ){
			try{
				co_await ArchiveAwait{ DailyFile(), _path, _tz };
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