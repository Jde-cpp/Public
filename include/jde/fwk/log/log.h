#pragma once
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include "../exports.h"
#include "../io/crc.h"
#include "../process/process.h"
#include "logTags.h"
#include "Entry.h"
#ifndef _MSC_VER
	#include <signal.h>
#endif

#define Φ Γ auto
namespace Jde{
	Φ ToString( ELogLevel l )ι->string;
	α ToLogLevel( sv op )ι->ELogLevel;
	α LogLevelStrings()ι->const std::array<sv,7>;
}


#define LOGSL(level, sl, tags, message,...) \
	if( Logging::ShouldLog(level, (ELogTags)tags) && !Process::Finalizing() ){\
 		Logging::Log( level, (ELogTags)tags, sl, message __VA_OPT__(,) __VA_ARGS__ );\
	}
#define LOG(level, tags, message,...) LOGSL( level, SRCE_CUR, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define CRITICAL(message,...) CRITICALT( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define CRITICALT(tags, message,...) LOG( Jde::ELogLevel::Critical, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define ERR( message, ... ) ERRT( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define ERRT(tags, message,...) LOG( ELogLevel::Error, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define WARN(message,...) WARNT( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define WARNT(tags, message,...) LOG( ELogLevel::Warning, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define INFO(message,...) INFOT( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define INFOT(tags, message,...) LOG( ELogLevel::Information, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define DBG(message,...) DBGT( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define DBGT(tags, message,...) LOG( ELogLevel::Debug, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define DBGSL(message,...) LOGSL( ELogLevel::Debug, sl, _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define RETURN_IF( predicate, level, message, ... ) if( predicate ){ LOG(level, _tags, message __VA_OPT__(,) __VA_ARGS__); return; }
#define TRACE(message,...) TRACET( _tags, message __VA_OPT__(,) __VA_ARGS__ )
#define TRACET(tags, message,...) LOG( ELogLevel::Trace, tags, message __VA_OPT__(,) __VA_ARGS__ )
#define TRACESL(message,...) LOGSL( ELogLevel::Trace, sl, _tags, message __VA_OPT__(,) __VA_ARGS__ )

namespace Jde::Logging{
	struct ILogger; struct MemoryLog;
	Φ LogException( const IException& e )ι->void;
	Φ DestroyLoggers( bool terminate )->void;
	Φ Loggers()->const vector<up<ILogger>>&;
	Ŧ GetLogger()ε->T&;
	Ŧ FindLogger()ι->T*;
	Φ AddLogger( up<ILogger>&& logger )ι->ILogger*;
	Φ Init()ι->void;
	Φ ClientMinLevel()ι->ELogLevel;
	namespace Proto{class Status;}
	Φ SetStatus( const vector<string>& values )ι->void;
	α SetLogLevel( ELogLevel client, ELogLevel server )ι->void;
	α GetStatus()ι->up<Proto::Status>;
}
namespace Jde{
	Ŧ Logging::FindLogger()ι->T*{
		for( auto& logger : Loggers() ){
			if( auto log = dynamic_cast<T*>( logger.get() ) )
				return log;
		}
		return nullptr;
	}
	Ŧ Logging::GetLogger()ε->T&{
		auto p = FindLogger<T>();
		if( !p )
			throw std::runtime_error( Ƒ("Logger of type {} not found.", typeid(T).name()) );
		return *p;
	}
}
#undef Φ
#endif