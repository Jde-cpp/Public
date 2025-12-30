#include <jde/fwk/log/Logger.h>

namespace Jde{
	concurrent_flat_set<StringMd5> _loggedEntries;
	α Logging::MarkLogged( StringMd5 id )ι->bool{
		return _loggedEntries.insert(id);
	}
	α Logging::CanBreak()ι->bool{ return Process::IsDebuggerPresent(); }

	α Logging::Log( const Entry& entry )ι->void{
		if( Process::Finalizing() || !ShouldLog(entry.Level, entry.Tags) )
			return;
		for( auto& logger : Logging::Loggers() ){
			try{
				if( logger->ShouldLog(entry.Level, entry.Tags) )
					logger->Write( entry );
			}
			catch( const fmt::format_error& e ){
				CRITICALT( ELogTags::App, "could not log entry '{}' error: '{}'", entry.Text, e.what() );
			}
		}
		BREAK_IF( entry.Tags<=ELogTags::Write && entry.Level>=BreakLevel() );//don't want to break for opc server.
	}
	α Logging::Log( const Entry& entry, uint32 appPK, uint32 instancePK )ι->void{
		if( Process::Finalizing() || !ShouldLog(entry.Level, entry.Tags) )
			return;
		for( auto& logger : Logging::Loggers() ){
			try{
				if( logger->ShouldLog(entry.Level, entry.Tags) )
					logger->Write( entry, appPK, instancePK );
			}
			catch( const fmt::format_error& e ){
				CRITICALT( ELogTags::App, "could not log entry '{}' error: '{}'", entry.Text, e.what() );
			}
		}
	}
}