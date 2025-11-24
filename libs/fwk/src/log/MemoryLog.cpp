#include <jde/fwk/log/MemoryLog.h>

#define let const auto
namespace Jde{
	Ω memoryLog()ε->Logging::MemoryLog&{
		for( let& logger : Logging::Loggers() ){
			if( let pMemoryLog = dynamic_cast<Logging::MemoryLog*>(logger.get()); pMemoryLog )
				return *pMemoryLog;
		}
		throw Exception( "No MemoryLog logger registered." );
	}
	α Logging::ClearMemory()ι->void{ memoryLog().Clear(); }
	α Logging::Find( StringMd5 entryId )ι->vector<Logging::Entry>{ return memoryLog().Find( entryId ); }
	α Logging::Find( function<bool(const Logging::Entry&)> f )ι->vector<Logging::Entry>{ return memoryLog().Find( f ); }
}
namespace Jde::Logging{
	α MemoryLog::Write( const Entry& m )ι->void{
		_entries.emplace_back( move(m) );
	}
	α MemoryLog::Write( ILogger& logger )ι->void{
		auto entries = _entries.copy(); //Write may log additional _entries.
		for( let& entry : entries ){
			if( logger.ShouldLog(entry.Level, entry.Tags) )
				logger.Write( entry );
		}
	}
	α MemoryLog::Find( StringMd5 id )ι->vector<Logging::Entry>{
		vector<Logging::Entry> y;
		_entries.visit( [&](let& entry){
			if( entry.Id() == id )
				y.push_back(entry);
		});
		return y;
	}
	α MemoryLog::Find( function<bool(const Logging::Entry&)> f )ι->vector<Logging::Entry>{
		vector<Logging::Entry> y;
		_entries.visit( [&](let& entry){
			if( f(entry) )
				y.push_back(entry);
		});
		return y;
	}

	α MemoryLog::Find( string text )ι->vector<Logging::Entry>{
		return Find( [&](let& entry){
			return entry.Message()==text;
		} );
	}
}