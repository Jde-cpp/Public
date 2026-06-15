#include "ArchiveAwait.h"
#include <chrono>
#include <filesystem>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/app/log/DailyLoadAwait.h>

#include <boost/uuid/uuid_io.hpp>

#define let const auto

namespace Jde::App{
	static constexpr ELogTags _tags{ ELogTags::ExternalLogger };
	using Protobuf::ToGuid;
	α ArchiveAwait::Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task{
		vector<App::Log::Proto::FileEntry> entries;
		try{
			entries = co_await DailyLoadAwait{ _dailyFile }; //TODO lock until done
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
				strings[ToGuid( entry.str().id() )] = move( *entry.mutable_str() );
			else{
				let day = Chrono::LocalYMD( Protobuf::ToTimePoint(entry.entry().time()), _tz );
				*archives[day].add_entries() = move( *entry.mutable_entry() );
			}
		}
		for( auto&& [ymd,archive] : archives ){
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
		Save( move(archives) );
	}
	struct ArchiveFileAwait final : VoidAwait{
		ArchiveFileAwait( year_month_day ymd, const fs::path& root, App::Log::Proto::ArchiveFile&& archive, SL sl )ε;
		α Suspend()ι->void override;
	private:
		α Append()ι->TAwait<string>::Task;
		α Save( App::Log::Proto::ArchiveFile&& values )ι->VoidAwait::Task;
		App::Log::Proto::ArchiveFile _archive;
		fs::path _file;
	};
	α ArchiveAwait::Save( flat_map<year_month_day, App::Log::Proto::ArchiveFile> archives )ι->VoidAwait::Task{
		for( auto&& [ymd,archive] : archives ){
			try{
				co_await ArchiveFileAwait{ ymd, _path, move(archive), _sl };
			}
			catch( exception& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
		fs::remove( _dailyFile );
		Resume();
	}
	Ω getFile( year_month_day ymd, const fs::path& root )ι->fs::path{
		let dir = root/std::to_string( (int)ymd.year() )/std::to_string( (unsigned)ymd.month() )/std::to_string( (unsigned)ymd.day() );
		if( !fs::exists(dir) )
			fs::create_directories( dir );
		return dir/"archive.binpb";
	}
	ArchiveFileAwait::ArchiveFileAwait( year_month_day ymd, const fs::path& root, App::Log::Proto::ArchiveFile&& archive, SL sl )ε:
		VoidAwait{ sl },
		_archive{ move(archive) },
		_file{ getFile(ymd, root) }
	{}

	α ArchiveFileAwait::Suspend()ι->void{
		if( fs::exists(_file) )
			Append();
		else
			Save( move(_archive) );
	}
	α ArchiveFileAwait::Append()ι->TAwait<string>::Task{
		string content;
		try{
			content = co_await IO::ReadAwait( _file );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			co_return;
		}
		std::map<uuid,App::Log::Proto::String> args, templates, files, functions;
		std::map<TimePoint,App::Log::Proto::LogEntryFile> entries;
		auto existing = Protobuf::Deserialize<App::Log::Proto::ArchiveFile>( move(content) );
		auto addEntries = [&entries]( App::Log::Proto::ArchiveFile& af ){
			for( int i=0; i<af.entries_size(); ++i ){
				auto& entry = *af.mutable_entries( i );
				entries.emplace_hint( entries.end(), Protobuf::ToTimePoint(entry.time()), move(entry) );
			}
		};
		addEntries( existing );
		addEntries( _archive );
#define ADD_STRINGS( collection, af ) \
		for( int i=0; i<af.collection##_size(); ++i ){ \
			auto& s = *af.mutable_##collection( i ); \
			collection[ToGuid( s.id() )] = move( s ); \
		}
		ADD_STRINGS( args, existing );
		ADD_STRINGS( args, _archive );
		ADD_STRINGS( templates, existing );
		ADD_STRINGS( templates, _archive );
		ADD_STRINGS( files, existing );
		ADD_STRINGS( files, _archive );
		ADD_STRINGS( functions, existing );
		ADD_STRINGS( functions, _archive );

		App::Log::Proto::ArchiveFile cumulative;
		for( auto& [_,entry] : entries )
			*cumulative.add_entries() = move( entry );
		for( auto& [_,s] : args )
			*cumulative.add_args() = move( s );
		for( auto& [_,s] : templates )
			*cumulative.add_templates() = move( s );
		for( auto& [_,s] : files )
			*cumulative.add_files() = move( s );
		for( auto& [_,s] : functions )
			*cumulative.add_functions() = move( s );

		Save( move(cumulative) );
	}
	α ArchiveFileAwait::Save( App::Log::Proto::ArchiveFile&& values )ι->VoidAwait::Task{
		try{
			co_await IO::WriteAwait( move(_file), Protobuf::ToString(values), true, _tags );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ArchiveLoadAwait::Suspend()ι->void{
		try{
			ArchiveFile archive{ _query.Filter(), move(_dailyFile) };
			let isComplete = archive.IsComplete( _query );
			TRACE( "daily file entries: {}, templates: {}, files: {}, functions: {}, args: {}, complete: {}",
				archive.Entries.size(),
				archive.Templates.size(),
				archive.Files.size(),
				archive.Functions.size(),
				archive.Args.size(),
				isComplete );
			if( isComplete )
				Resume( move(archive) );
			else
			 	LoadArchives( move(archive) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ArchiveLoadAwait::ArchiveFiles()ι->flat_map<year_month_day, fs::path>{
		flat_map<year_month_day, fs::path> y;
		for( let& yearEntry : fs::directory_iterator(_root) ){
			if( let yearV = yearEntry.is_directory() ? Str::TryTo<year>(yearEntry.path().stem().string()) : nullopt; yearV ){
				for( let& monthEntry : fs::directory_iterator(yearEntry.path()) ){
					if( let monthV = monthEntry.is_directory() ? Str::TryTo<month>(monthEntry.path().stem().string()) : nullopt; monthV ){
						for( let& dayEntry : fs::directory_iterator(monthEntry.path()) ){
							if( let dayV = dayEntry.is_directory() ? Str::TryTo<day>(dayEntry.path().stem().string()) : nullopt; dayV ){
								let ymd = year_month_day{ *yearV, *monthV, *dayV };
								if( ymd.ok() && fs::exists(dayEntry.path()/"archive.binpb") )
									y[ymd] = dayEntry.path()/"archive.binpb";
							}
						}
					}
				}
			}
		}
		return y;
	}

	α ArchiveLoadAwait::LoadArchives( ArchiveFile archive )ι->StringAwait::Task{
		try{
			auto process = [this]( ArchiveFile& archive, string&& content )ι->bool {
				archive.Append( _query, Protobuf::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
				return archive.IsComplete( _query );
			};

			let files = ArchiveFiles();
			if( !files.size() ){
				Resume( move(archive) );
				co_return;
			}
			let localStart = _startTime.has_value() ? year_month_day{ floor<days>(_tz.to_local(*_startTime)) } : files.begin()->first;
			let localEnd = _endTime.has_value() ? year_month_day{ ceil<days>(_tz.to_local(*_endTime)) } : files.rbegin()->first;
			auto test = [localStart, localEnd]( year_month_day fileDate )ι->bool {
				return fileDate >= localStart && fileDate <= localEnd;
			};

			if( let& ob = _query.OrderByJson(); ob.size() && ob[0].first=="time" && ob[0].second ){
				for( auto&& [ymd,file] : files ){
					if( !test(ymd) )
						continue;
					auto content = co_await IO::ReadAwait( file );
					if( process(archive, move(content)) ){
						Resume( move(archive) );
						co_return;
					}
				}
			}
			else{
				for( auto it = files.rbegin(); it!=files.rend(); ++it ){
					if( !test(it->first) )
						continue;
					auto content = co_await IO::ReadAwait( it->second );
					if( process(archive, move(content)) ){
						Resume( move(archive) );
						co_return;
					}
				}
			}
			Resume( move(archive) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}