#include "LogAwait.h"
#include <jde/fwk/chrono.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/log/DailyLoadAwait.h>
#include "ArchiveAwait.h"

#define let const auto

namespace Jde::App{
	α LogAwait::Execute()ι->TAwait<vector<App::Log::Proto::FileEntry>>::Task{
		let protoLog = Logging::FindLogger<ProtoLog>();
		try{
			THROW_IFX( !protoLog, "No logger running." );
			let& filters = _ql.Filter();
			optional<TimePoint> startTime, endTime;
			if( auto time = filters.ColumnFilters.find("time"); time!=filters.ColumnFilters.end() ){
				let& criteria = time->second;
				for( let& crit : criteria ){
					if( crit.Operator==DB::EOperator::Greater )
						startTime = Chrono::ToTimePoint( string{crit.Value.as_string()} );
					else if( crit.Operator==DB::EOperator::Less )
						endTime = Chrono::ToTimePoint( string{crit.Value.get_string()} );
				}
			}
			auto entries = !endTime || *endTime > protoLog->DailyFileStart()
				? co_await DailyLoadAwait{ protoLog->DailyFile() }
				: vector<App::Log::Proto::FileEntry>{};
			ReadArchive( startTime, endTime, move(entries) );
		}
		catch( std::exception& e ){
			ResumeExp( move(e) );
		}
	}
	α LogAwait::ReadArchive( optional<TimePoint> startTime, optional<TimePoint> endTime, vector<App::Log::Proto::FileEntry> entries )ι->TAwait<ArchiveFile>::Task{
		try{
			let& protoLog = Logging::GetLogger<ProtoLog>();
			auto archive = co_await ArchiveLoadAwait{ startTime, endTime, _ql, protoLog.TimeZone(), protoLog.Root() };
			archive.Append( _ql.Filter(), move(entries) );
			Resume( move(archive) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}