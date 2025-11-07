#include "ArchiveAwait.h"
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/io/proto.h>
#include <jde/app/log/DailyLoadAwait.h>

#define let const auto

namespace Jde::App{
	static constexpr ELogTags _tags{ ELogTags::ExternalLogger | ELogTags::IO };
	using Jde::Proto::ToGuid;
	α ArchiveAwait::Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task{
		vector<App::Log::Proto::FileEntry> entries;
		try{
			entries = co_await DailyLoadAwait{ _dailyFile };
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
	α ArchiveAwait::Append( const fs::path archiveFile, App::Log::Proto::ArchiveFile append )ι->TAwait<string>::Task{
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
	α ArchiveAwait::Save( fs::path file, App::Log::Proto::ArchiveFile values )ι->VoidAwait::Task{
		try{
			co_await IO::WriteAwait( move(file), Jde::Proto::ToString(values), true, _tags );
			fs::remove( _dailyFile );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ArchiveLoadAwait::Execute()ι->StringAwait::Task{
		ArchiveFile archive{};
//		ArchiveFile a2{ move(archive) };
//		ArchiveFile a3 = move(a2);
		try{
			auto iterate = []( fs::path path, function<int(int)> valid )->vector<int> {
				vector<int> result;
				for( let& entry : fs::directory_iterator(path) ){
					if( let value = entry.is_directory() ? valid(Str::TryTo<int>(entry.path().stem().string()).value_or(0)) : 0; value )
						result.push_back( value );
				}
				return result;
			};
			let localStart = _startTime.has_value() ? year_month_day{floor<days>(_tz.to_local(*_startTime))} : year_month_day{ year{1970},month{1},day{1} };
			let localEnd = _endTime.has_value() ? year_month_day{floor<days>(_tz.to_local(*_endTime))} : year_month_day{ year{2099},month{12},day{31} };
			auto validYear = [localStart,localEnd]( int year )ι->int {
				if( year < (int)localStart.year()
				 || year > (int)localEnd.year() ){
					return 0;
				}
				return year>1970 ? year : 0;
			};
			for( let year : iterate(_root, validYear) ){
				let yearPath = _root/std::to_string(year);
				auto validMonth = [year,localStart,localEnd]( int m )ι->uint{
					unsigned month = (unsigned)m;
					if( year == (int)localStart.year() && month < (unsigned)localStart.month() )
						return 0;
					if( year == (int)localEnd.year() && month > (unsigned)localEnd.month() )
						return 0;
					return month>0 && month<=12 ? month : 0;
				};
				for( let& m : iterate(yearPath, validMonth) ){
					unsigned month = (unsigned)m;
					let monthPath = yearPath/std::to_string(month);
					auto validDay = [year,month,localStart,localEnd]( int day )ι->uint{
						if( year == (int)localStart.year() && month == (unsigned)localStart.month() && ((unsigned)day) < (unsigned)localStart.day() )
							return 0;
						if( year == (int)localEnd.year() && month == (unsigned)localEnd.month() && ((unsigned)day) > (unsigned)localEnd.day() )
							return 0;
						return day>0 && day<=31 ? day : 0;
					};
					for( let& day : iterate(monthPath, validDay) ){
						let file = monthPath/std::to_string(day)/"archive.binpb";
						if( !fs::exists(file) )
							continue;
						auto content = co_await IO::ReadAwait( file );
						archive.Append( _query, Jde::Proto::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
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