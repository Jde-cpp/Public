#pragma once
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include "../exports.h"
#include "../collections/Vector.h"
#include "../io/crc.h"
#include "../process.h"
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

#define MY_FILE __FILE__

#define CRITICAL(message,...) Critical{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define ERR( message, ... ) Error{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define WARN(message,...) Warning{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define INFO(message,...) Information{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define DBG(message,...) Debug{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define DBGSL(message,...) Debug{ sl, _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define RETURN_IF( predicate, severity, message, ... ) if( predicate ){ Log(severity, _tags, SRCE_CUR, message __VA_OPT__(,) __VA_ARGS__); return; }
#define TRACE(message,...) Trace{ _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define TRACESL(message,...) Trace{ sl, _tags, message __VA_OPT__(,) __VA_ARGS__ }
#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&

namespace Jde::Logging{
	struct ILogger;
	Φ LogException( const IException& e )ι->void;
	Φ DestroyLoggers()->void;
	Φ Loggers()->const vector<up<ILogger>>&;
	Φ AddLogger( up<ILogger>&& logger )->void;
	Φ Initialize()ι->void;
	Φ ClientMinLevel()ι->ELogLevel;
	namespace Proto{class Status;}
	Φ SetStatus( const vector<string>& values )ι->void;
	α SetLogLevel( ELogLevel client, ELogLevel server )ι->void;
	α GetStatus()ι->up<Proto::Status>;
}
#undef SOURCE
#undef let
#undef FormatString
#undef ARGS
#undef Φ
#endif