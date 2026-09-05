#include <jde/fwk/log/log.h>

namespace Jde{
	concurrent_flat_set<StringMd5> _loggedEntries;
	α Logging::MarkLogged( StringMd5 id )ι->bool{
		return _loggedEntries.insert(id);
	}

	//The one fan-out loop behind both Log(Entry) overloads - they used to be copies differing only in the Write call.
	//Returns false when the entry was skipped (finalizing, or below every logger's level) so the caller's BREAK_IF
	//keeps its old early-return behaviour.
	Ṫ log( const Logging::Entry& entry, T&& write )ι->bool{
		if( Process::Finalizing() || !Logging::ShouldLog(entry.Level, entry.Tags) )
			return false;
		for( auto& logger : Logging::Loggers() ){
			try{
				if( logger->ShouldLog(entry.Level, entry.Tags) )
					write( *logger );
			}
			catch( const fmt::format_error& e ){
				CRITICALT( ELogTags::App, "could not log entry '{}' error: '{}'", entry.Text, e.what() );
			}
		}
		return true;
	}
	α Logging::Log( const Entry& entry )ι->void{
		[[maybe_unused]] const bool logged = log( entry, [&](ILogger& l){ l.Write(entry); } );
		BREAK_IF( logged && entry.Tags<=ELogTags::Write && entry.Level>=BreakLevel() );//don't want to break for opc server.
	}
	α Logging::Log( const Entry& entry, uint32 appPK, uint32 instancePK )ι->void{
		log( entry, [&](ILogger& l){ l.Write(entry, appPK, instancePK); } );//no BREAK_IF here: this is a forwarded entry from another instance, not this process's own.
	}
}