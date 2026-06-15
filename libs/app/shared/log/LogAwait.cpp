#include "LogAwait.h"
#include <jde/fwk/chrono.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/log/DailyLoadAwait.h>
#include "ArchiveAwait.h"

#define let const auto
namespace Jde::App{
	static constexpr ELogTags _tags{ ELogTags::ExternalLogger };

	α LogAwait::Suspend()ι->void{
		try{
			_dailyFileStart = Logging::GetLogger<ProtoLog>().DailyFileStart();
			let& filters = _ql.Filter();
			if( auto time = filters.ColumnFilters.find("time"); time!=filters.ColumnFilters.end() ){
				let& criteria = time->second;
				for( let& crit : criteria ){
					if( crit.Operator==DB::EOperator::Greater )
						_startTime = Chrono::ToTimePoint( string{crit.Value.as_string()} );
					else if( crit.Operator==DB::EOperator::Less )
						_endTime = Chrono::ToTimePoint( string{crit.Value.get_string()} );
				}
			}
			let& ob = _ql.OrderByJson();
			let timeAsc = ob.size() && ob[0].first=="time" && ob[0].second;
			if( ShouldReadLocal() && !timeAsc )
				ReadLocal( nullopt );
			else
				ReadArchive( {} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α LogAwait::ReadArchive( vector<App::Log::Proto::FileEntry> entries )ι->TAwait<ArchiveFile>::Task{
		try{
			TRACE( "Daily item count: {}", entries.size() );
			let loadedDaily = entries.size()>0;
			let& protoLog = Logging::GetLogger<ProtoLog>();
			auto archive = co_await ArchiveLoadAwait{ _startTime, _endTime, _ql, protoLog.TimeZone(), protoLog.Root(), move(entries) };
			if( !loadedDaily && !archive.IsComplete(_ql) && ShouldReadLocal() )
				ReadLocal( move(archive) );
			else
				Resume( move(archive) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α LogAwait::ReadLocal( optional<ArchiveFile> archive )ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task{
		try{
			auto dailyEntries = co_await DailyLoadAwait{ Logging::GetLogger<ProtoLog>().DailyFile() };
			TRACE( "Daily file item count: {}", dailyEntries.size() );
			if( archive ){
				archive->Append( _ql.Filter(), move(dailyEntries) );
				Resume( move(*archive) );
			}
			else
				ReadArchive( move(dailyEntries) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}